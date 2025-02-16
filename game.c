#include "libdragon.h"
#include "scene.h"

#define ROCK_MAX 12
#define BULLET_MAX 25
#define EXPLODE_MAX ROCK_MAX+1

typedef struct player_s {
    float x;
    float y;
    float angle;
    float speed;
    float damage_timer;
    float reset_timer;
} player_t;

typedef struct rock_s {
    bool active;
    float x;
    float y;
    float vel_x;
    float vel_y;
    float angle;
    int size;
} rock_t;

typedef struct bullet_s {
    bool active;
    float x;
    float y;
    float angle;
    float speed;
} bullet_t;

typedef struct explode_s {
    bool active;
    float x;
    float y;
    float time;
    float max_time;
    float radius;
} explode_t;

static int rock_radius[3] = { 24, 12, 6 };
static rock_t rock_all[ROCK_MAX];
static bullet_t bullet_all[BULLET_MAX];
static player_t player;
static explode_t explode_all[EXPLODE_MAX];
static sprite_t *spr_rock[3];
static sprite_t *spr_bullet;
static sprite_t *spr_ship;
static wav64_t *sfx_explode[3];
static wav64_t *sfx_fire;
static float game_time;
static bool game_pause;

static void wrap_pos(float *x, float *y, float border_w, float border_h)
{
    int disp_w = display_get_width();
    int disp_h = display_get_height();
    if(*x < -border_w) {
        *x += disp_w+(border_w*2);
    } else if(*x > disp_w+border_w) {
        *x -= disp_w+(border_w*2);
    }
    if(*y < -border_h) {
        *y += disp_h+(border_h*2);
    } else if(*y > disp_h+border_h) {
        *y -= disp_h+(border_h*2);
    }
}

static bool check_circle_col(float x1, float y1, float r1, float x2, float y2, float r2)
{
    float dx = x2-x1;
    float dy = y2-y1;
    return ((dx*dx)+(dy*dy)) < ((r1+r2)*(r1+r2));
}

static void explode_init(void)
{
    for(int i=0; i<EXPLODE_MAX; i++) {
        explode_all[i].active = false;
    }
}

static void explode_create(float x, float y, float radius, float max_time)
{
    for(int i=0; i<EXPLODE_MAX; i++) {
        explode_t *explode = &explode_all[i];
        if(!explode->active) {
            explode->active = true;
            explode->x = x;
            explode->y = y;
            explode->radius = radius;
            explode->time = 0;
            explode->max_time = max_time;
            return;
        }
    }
}

static void explode_update(float dt)
{
    for(int i=0; i<EXPLODE_MAX; i++) {
        explode_t *explode = &explode_all[i];
        if(explode->active) {
            explode->time += dt;
            if(explode->time >= explode->max_time) {
                explode->active = false;
            }
        }
    }
}

static void explode_draw(void)
{
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    
    for(int i=0; i<EXPLODE_MAX; i++) {
        explode_t *explode = &explode_all[i];
        if(explode->active) {
            float t = explode->time/explode->max_time;
            float alpha = (t < 0.3f) ? 255 : (((1-t)/0.7f)*255);
            rdpq_set_prim_color(RGBA32(255, 255, 255, alpha));
            for(int j=0; j<8; j++) {
                float radius = explode->radius*t;
                float x = explode->x+(radius*cosf(((j+0.5f)*M_PI*2)/8));
                float y = explode->y+(radius*sinf(((j+0.5f)*M_PI*2)/8));
                
                rdpq_fill_rectangle(x-2, y-2, x+2, y+2);
            }
        }
    }
}

static void rock_init(void)
{
    for(int i=0; i<ROCK_MAX; i++) {
        rock_all[i].active = false;
        rock_all[i].size = 0;
    }
}

static void rock_create(float x, float y, float vel_x, float vel_y, int size)
{
    for(int i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(!rock->active) {
            rock->active = true;
            rock->x = x;
            rock->y = y;
            rock->vel_x = vel_x;
            rock->vel_y = vel_y;
            rock->size = size;
            return;
        }
    }
}

static void rock_start_create(void)
{
    for(int i=0; i<3; i++) {
        float angle = (i*2*M_PI)/3;
        float x = player.x+(120*cosf(angle));
        float y = player.y+(120*sinf(angle));
        float vel_x = 75*cosf(angle+(M_PI/5));
        float vel_y = 75*sinf(angle+(M_PI/5));
        rock_create(x, y, vel_x, vel_y, 0);
    }
}

static void rock_update(float dt)
{
    int num_rocks =  0;
    for(int i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(rock->active) {
            num_rocks++;
            rock->x += rock->vel_x*dt;
            rock->y += rock->vel_y*dt;
            rock->angle += 1.0f*dt;
            wrap_pos(&rock->x, &rock->y, rock_radius[rock->size], rock_radius[rock->size]);
        }
    }
    if(num_rocks == 0) {
        rock_start_create();
    }
}

static void rock_draw(void)
{
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(1); // colorkey (draw pixel with alpha >= 1)
    for(int i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(rock->active) {
            rdpq_sprite_blit(spr_rock[rock->size], rock->x, rock->y, &(rdpq_blitparms_t){
                .cx = rock_radius[rock->size],
                .cy = rock_radius[rock->size],
                .theta = rock->angle
            });
        }
    }
}

static void bullet_init(void)
{
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_all[i].active = false;
    }
}

static void bullet_spawn(void)
{
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_t *bullet = &bullet_all[i];
        if(!bullet->active) {
            bullet->active = true;
            bullet->x = player.x-(6*sinf(player.angle));
            bullet->y = player.y-(6*cosf(player.angle));
            bullet->angle = player.angle;
            bullet->speed = 300;
            return;
        }
    }
}

static void bullet_update(float dt)
{
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_t *bullet = &bullet_all[i];
        
        if(bullet->active) {
            for(int j=0; j<ROCK_MAX; j++) {
                rock_t *rock = &rock_all[j];
                if(rock->active) {
                    if(check_circle_col(bullet->x, bullet->y, 0, rock->x, rock->y, rock_radius[rock->size])) {
                        wav64_play(sfx_explode[rock->size], 1);
                        explode_create(rock->x, rock->y, rock_radius[rock->size]*2, 0.7f);
                        if(rock->size != 2) {
                            int new_size = rock->size+1;
                            float speed_x = 75*sinf(bullet->angle);
                            float speed_y = 75*cosf(bullet->angle);
                            rock_create(rock->x, rock->y, speed_y, -speed_x, new_size);
                            rock_create(rock->x, rock->y, -speed_y, speed_x, new_size);
                        }
                        rock->active = false;
                        bullet->active = false;
                        continue;
                    }
                }
            }
            bullet->x -= bullet->speed*sinf(bullet->angle)*dt;
            bullet->y -= bullet->speed*cosf(bullet->angle)*dt;
            if(bullet->x < -16 || bullet->x >= 16+display_get_width() || bullet->y < -16 || bullet->y >= 16+display_get_height()) {
                bullet->active = false;
            }
        }
    }
}

static void bullet_draw(void)
{
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(1); // colorkey (draw pixel with alpha >= 1)
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_t *bullet = &bullet_all[i];
        if(bullet->active) {
            rdpq_sprite_blit(spr_bullet, bullet->x, bullet->y, &(rdpq_blitparms_t){
                .cx = 3,
                .cy = 6,
                .theta = bullet->angle,
            });
        }
    }
}

static void player_reset(void)
{
    player.x = display_get_width()/2;
    player.y = display_get_height()/2;
    player.angle = 0;
    player.speed = 0;
    player.damage_timer = 1.0f;
    player.reset_timer = 0;
}

static void player_update(float dt)
{
    if(player.reset_timer > 0) {
        player.reset_timer -= dt;
        if(player.reset_timer <= 0) {
            player_reset();
        }
        return;
    }
    joypad_buttons_t btn = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_buttons_t btn_press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_inputs_t stick = joypad_get_inputs(JOYPAD_PORT_1);
    if(stick.stick_x < -16 || stick.stick_x > 16) {
        player.angle -= stick.stick_x*0.05f*dt;
        if(player.angle > (2*M_PI)) {
            player.angle -= (2*M_PI);
        }
        if(player.angle < 0) {
            player.angle += (2*M_PI);
        }
    }
    if(btn.a) {
        player.speed += 2.0f;
        if(player.speed > 140.0f) {
            player.speed = 140.0f;
        }
    }
    if(btn.b) {
        player.speed -= 2.0f;
        if(player.speed < 0) {
            player.speed = 0;
        }
    }
    if(btn_press.z) {
        wav64_play(sfx_fire, 0);
        bullet_spawn();
    }
    bool in_rock = false;
    for(int i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(rock->active) {
            if(check_circle_col(player.x, player.y, 8, rock->x, rock->y, rock_radius[rock->size])) {
                if(player.damage_timer <= 0) {
                    player.reset_timer = 0.5f;
                    wav64_play(sfx_explode[1], 1);
                    explode_create(player.x, player.y, 24, 0.5f);
                } else {
                    in_rock = true;
                }
            }
        }
    }
    
    if(player.damage_timer > 0 && !in_rock) {
        player.damage_timer -= dt;
        if(player.damage_timer <= 0) {
            player.damage_timer = 0;
        }
    }
    player.x -= sinf(player.angle)*player.speed*dt;
    player.y -= cosf(player.angle)*player.speed*dt;
    wrap_pos(&player.x, &player.y, 20, 20);
}

static void player_draw(void)
{
    if(player.reset_timer <= 0) {
        if(player.damage_timer < 0.1f || fmodf(game_time, 0.1f) > 0.05f) {
            rdpq_sprite_blit(spr_ship, player.x, player.y, &(rdpq_blitparms_t){
                .cx = 8,
                .cy = 15,
                .theta = player.angle
            });
        }
    }
}

static void init(void)
{
    spr_rock[0] = sprite_load("rom:/rock_big.sprite");
    spr_rock[1] = sprite_load("rom:/rock_mid.sprite");
    spr_rock[2] = sprite_load("rom:/rock_small.sprite");
    spr_ship = sprite_load("rom:/ship.sprite");
    spr_bullet = sprite_load("rom:/bullet.sprite");
    sfx_explode[0] = wav64_load("rom:/explode_big.wav64", NULL);
    sfx_explode[1] = wav64_load("rom:/explode_mid.wav64", NULL);
    sfx_explode[2] = wav64_load("rom:/explode_small.wav64", NULL);
    sfx_fire = wav64_load("rom:/fire.wav64", NULL);
    game_time = 0;
    game_pause = false;
    
    explode_init();
    rock_init();
    bullet_init();
    
    player_reset();
    player.damage_timer = 0;
    
    rock_start_create();
}

static void draw(void)
{
    bullet_draw();
    player_draw();
    rock_draw();
    explode_draw();
}

static void update(float dt)
{
    joypad_buttons_t btn_press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(btn_press.start) {
        game_pause = !game_pause;
    }
    if(!game_pause) {
        game_time += dt;
        player_update(dt);
        rock_update(dt);
        bullet_update(dt);
        explode_update(dt);
    }
    
}

static void cleanup(void)
{
    
}

scene_data_t game_scene = { init, draw, update, cleanup };