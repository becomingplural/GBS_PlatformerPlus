#pragma bank 255

#include "camera.h"
#include "actor.h"

INT16 camera_x;
INT16 camera_y;
BYTE camera_offset_x;
BYTE camera_offset_y;
UBYTE camera_deadzone_x;
UBYTE camera_deadzone_y;
UBYTE camera_settings;
UBYTE plat_camera_follow;
UBYTE plat_camera_lead;
UBYTE plat_camera_catchup;

void camera_init() BANKED {
    camera_x = camera_y = 0;
    camera_offset_x = camera_offset_y = 0;
    camera_reset();
}

void camera_reset() BANKED {
    camera_deadzone_x = camera_deadzone_y = 0;
    camera_settings = CAMERA_LOCK_FLAG;

}

void camera_update() NONBANKED {
    /*
     I need to have an option for catchup speed. Otherwise the deadzone can get too wild.

    */

    if ((camera_settings & CAMERA_LOCK_X_FLAG)) {
        //Difference between player position and camera_x
        //The 8 in this formula is necessary for centering the camera, presumably because the sprite starts at x = 0
        WORD a_x = camera_x  - ((PLAYER.pos.x >> 4) + 8 + ((pl_vel_x >> 10)*plat_camera_lead));

        //Even out camera catchup to velocity but not when re-orienting.
        /*if (pl_vel_x != 0 && plat_camera_lead != 0){
                a_x -= PLAYER.bounds.right;
        }*/
        // Horizontal lock
        //Camera - Player = Negative when Player is to the right and camera is catching up by moving right
        if (plat_camera_follow & 1 && a_x < -camera_deadzone_x - camera_offset_x) {
            a_x = a_x + camera_deadzone_x - camera_offset_x;
            camera_x -= a_x >> plat_camera_catchup;
        } else if (plat_camera_follow & 2 && a_x > camera_deadzone_x - camera_offset_x) {
            a_x = a_x - camera_deadzone_x - camera_offset_x;
            camera_x -= a_x >> plat_camera_catchup;
        }
    }

    if ((camera_settings & CAMERA_LOCK_Y_FLAG)) {
        UWORD a_y = (PLAYER.pos.y >> 4) + 8;    
        // Vertical lock
        //Camera Downwards Movement
         if (plat_camera_follow & 4 && camera_y + camera_deadzone_y + camera_offset_y < a_y) { 
            camera_y = a_y - camera_deadzone_y - camera_offset_y;
        //Camera Upwards Movement
        } else if (plat_camera_follow & 8 && camera_y + camera_offset_y > a_y + camera_deadzone_y) { 
            camera_y = a_y + camera_deadzone_y - camera_offset_y;
        }
    }
}
