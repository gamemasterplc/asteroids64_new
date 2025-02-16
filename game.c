#include "libdragon.h"
#include "scene.h"

#define ROCK_MAX 12
#define BULLET_MAX 25

typedef struct player_s {
    float x;
    float y;
    float angle;
    float speed;
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

static int rock_diameter[3] = { 64, 32, 16 };
static rock_t rock_all[ROCK_MAX];
static bullet_t bullet_all[BULLET_MAX];
static player_t player;
static sprite_t *spr_rock[3];
static sprite_t *spr_bullet;
static sprite_t *spr_ship;

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
    int i;
    int num_rocks =  0;
    for(i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(rock->active) {
            num_rocks++;
            rock->x += rock->vel_x*dt;
            rock->y += rock->vel_y*dt;
            rock->angle += 1.0f*dt;
            wrap_pos(&rock->x, &rock->y, rock_diameter[rock->size]/2, rock_diameter[rock->size]/2);
        }
    }
    if(num_rocks == 0) {
        rock_start_create();
    }
}

static void bullet_init(void)
{
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_all[i].active = false;
    }
}

static bullet_t *bullet_new(void)
{
    for(int i=0; i<BULLET_MAX; i++) {
        if(!bullet_all[i].active) {
            bullet_all[i].active = true;
            return &bullet_all[i];
        }
    }
    return NULL;
}

static void bullet_delete(bullet_t *bullet)
{
    bullet->active = false;
}

static void bullet_spawn(void)
{
    bullet_t *bullet = bullet_new();
    if(!bullet) {
        return;
    }
    bullet->x = player.x-(8*sinf(player.angle));
    bullet->y = player.y-(8*cosf(player.angle));
    bullet->angle = player.angle;
    bullet->speed = 300;
}

static void bullet_update(float dt)
{
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_t *bullet = &bullet_all[i];
        
        if(bullet->active) {
            for(int j=0; j<ROCK_MAX; j++) {
                rock_t *rock = &rock_all[j];
                if(rock->active) {
                    if(check_circle_col(bullet->x, bullet->y, 0, rock->x, rock->y, rock_diameter[rock->size]/2)) {
                        if(rock->size != 2) {
                            int new_size = rock->size+1;
                            float speed_x = 75*sinf(bullet->angle);
                            float speed_y = 75*cosf(bullet->angle);
                            rock_create(rock->x, rock->y, speed_y, -speed_x, new_size);
                            rock_create(rock->x, rock->y, -speed_y, speed_x, new_size);
                        }
                        rock->active = false;
                        bullet_delete(bullet);
                        continue;
                    }
                }
            }
            bullet->x -= bullet->speed*sinf(bullet->angle)*dt;
            bullet->y -= bullet->speed*cosf(bullet->angle)*dt;
            if(bullet->x < -16 || bullet->x >= 16+display_get_width() || bullet->y < -16 || bullet->y >= 16+display_get_height()) {
                bullet_delete(bullet);
            }
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
    rock_init();
    bullet_init();
    
    player.x = display_get_width()/2;
    player.y = display_get_height()/2;
    player.angle = 0;
    player.speed = 0;
    rock_start_create();
}

static void draw(void)
{
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(1); // colorkey (draw pixel with alpha >= 1)
    
    for(int i=0; i<BULLET_MAX; i++) {
        bullet_t *bullet = &bullet_all[i];
        if(bullet->active) {
            rdpq_sprite_blit(spr_bullet, bullet->x, bullet->y, &(rdpq_blitparms_t){
                .cx = 4,
                .cy = 8,
                .theta = bullet->angle,
            });
        }
    }
    rdpq_sprite_blit(spr_ship, player.x, player.y, &(rdpq_blitparms_t){
        .cx = 11,
        .cy = 32,
        .theta = player.angle
    });
    for(int i=0; i<ROCK_MAX; i++) {
        rock_t *rock = &rock_all[i];
        if(rock->active) {
            rdpq_sprite_blit(spr_rock[rock->size], rock->x, rock->y, &(rdpq_blitparms_t){
                .cx = rock_diameter[rock->size]/2,
                .cy = rock_diameter[rock->size]/2,
                .theta = rock->angle
            });
        }
    }
}

static void player_update(float dt)
{
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
        bullet_spawn();
    }
    player.x -= sinf(player.angle)*player.speed*dt;
    player.y -= cosf(player.angle)*player.speed*dt;
    wrap_pos(&player.x, &player.y, 26, 26);
}


static void update(float dt)
{
    player_update(dt);
    rock_update(dt);
    bullet_update(dt);
}

static void cleanup(void)
{
    
}

scene_data_t game_scene = { init, draw, update, cleanup };