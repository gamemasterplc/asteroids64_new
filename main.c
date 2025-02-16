#include "libdragon.h"
#include "scene.h"

#include <malloc.h>
#include <math.h>



int main()
{
    //Init debug log
    debug_init_isviewer();
    debug_init_usblog();
    
    //Init audio
    audio_init(48000, 4);
	mixer_init(32);
    mixer_set_vol(0.7f);
    
    //Init various other functions
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    joypad_init();
    timer_init();
    dfs_init(DFS_DEFAULT_LOCATION);
    //Init RDPQ
    rdpq_init();
    rdpq_debug_start();
    
    scene_set_next(SCENE_game);
    while (1)
    {
        scene_init_call();
        while(!scene_check_done()) {
            //Clear display
            surface_t *disp = display_get();
            rdpq_attach_clear(disp, NULL);
            rdpq_set_mode_standard();
            scene_draw_call();
            //Finish frame
            rdpq_detach_show();
            //Read controller
            joypad_poll();
            scene_update_call(display_get_delta_time());
            //Update mixer
            mixer_try_play();
        }
        scene_kill_call();
    }
}
