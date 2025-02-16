#include "libdragon.h"
#include "scene.h"

#define DEFINE_SCENE(id) extern scene_data_t id##_scene;
#include "scene_list.h"
#undef DEFINE_SCENE

#define DEFINE_SCENE(id) &id##_scene,

static scene_data_t *scene_all[SCENE_MAX] = {
    #include "scene_list.h"
};
#undef DEFINE_SCENE

static scene_data_t *curr_scene;
static scene_data_t *next_scene;

void scene_set_next(SceneID scene)
{
    next_scene = scene_all[scene];
}

bool scene_check_done(void)
{
    return next_scene != NULL;
}

void scene_init_call(void)
{
    if(next_scene) {
        curr_scene = next_scene;
        next_scene = NULL;
    }
    if(curr_scene->init) {
        curr_scene->init();
    }
}

void scene_kill_call(void)
{
    rspq_wait();
    for(int i=0; i<32; i++) {
        mixer_ch_stop(i);
    }
    if(curr_scene->cleanup) {
        curr_scene->cleanup();
    }
}

void scene_update_call(float dt)
{
    if(curr_scene->update) {
        curr_scene->update(dt);
    }
}

void scene_draw_call(void)
{
    if(curr_scene->draw) {
        curr_scene->draw();
    }
}