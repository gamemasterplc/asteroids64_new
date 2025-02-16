#pragma once

#include <stdbool.h>

#define DEFINE_SCENE(id) SCENE_##id,

typedef enum {
    #include "scene_list.h"
    SCENE_MAX
} SceneID;

#undef DEFINE_SCENE

typedef struct {
    void (*init)(void);
    void (*draw)(void);
    void (*update)(float dt);
    void (*cleanup)(void);
} scene_data_t;

void scene_set_next(SceneID scene);
bool scene_check_done(void);
void scene_init_call(void);
void scene_kill_call(void);
void scene_update_call(float dt);
void scene_draw_call(void);