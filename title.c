#include "libdragon.h"
#include "scene.h"

#define FONT_MAIN 1

static rdpq_font_t *text_font;
static float start_time;
static bool start_disp;

static void init()
{
    text_font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO);
    rdpq_text_register_font(FONT_MAIN, text_font);
    start_time = 0;
    start_disp = true;
}

static void draw()
{
    int screen_w = display_get_width();
    int screen_h = display_get_height();
    rdpq_text_print(&(rdpq_textparms_t){.width=screen_w, .height=48, .align=ALIGN_CENTER},
        FONT_MAIN,
        0, 24,
        "Asteroids 64");
    if(start_disp) {
        rdpq_text_print(&(rdpq_textparms_t){.width=screen_w, .height=32, .align=ALIGN_CENTER, .valign=VALIGN_BOTTOM},
        FONT_MAIN,
        0, screen_h-48,
        "Press Start");
    }
    
}

static void update(float dt)
{
    joypad_buttons_t btn_press = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if(btn_press.start) {
        scene_set_next(SCENE_game);
        return;
    }
    start_time += dt;
    if(start_time > 0.5f) {
        start_time -= 0.5f;
        start_disp = true;
    } else if(start_time > 0.25f) {
        start_disp = false;
    }
}

static void cleanup()
{
    rdpq_font_free(text_font);
    rdpq_text_unregister_font(FONT_MAIN);
}

scene_data_t title_scene = { init, draw, update, cleanup };