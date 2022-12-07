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
    /*What this segment needs to do:
    1. Calculate the distance between the player and the camera
    2. Add current player velocity modifier
    3. Check if this is greater than the buffer (I need to know where the camera_x measures from to really understand this)
    4. Move the camera_x closer to its goal by a proportional amount
    
    //The problem is that the two cases where the camera needs to move left: to follow the player, and to correct after acceleration, need offsets to account for the sprite width but with opposite sign
    Need to SUBTRACT when going left, but ADD when camera is tracking back
    */

    //camera_deadzone_x = 16;
    if ((camera_settings & CAMERA_LOCK_X_FLAG)) {
        //Difference between player position and camera_x
        //The 8 in this formula is necessary for centering the camera, even without the deadzone/offset. Not sure what that's about.
        WORD a_x = camera_x  - ((PLAYER.pos.x >> 4) - 8 + ((pl_vel_x >> 10)*plat_camera_lead));

        //Even out camera catchup to velocity but not when re-orienting.
        if (pl_vel_x != 0){
                a_x -= PLAYER.bounds.right;
        }
        // Horizontal lock
        //Camera - Player = Negative when Player is to the right and camera is catching up by moving right
        if (plat_camera_follow & 1 && a_x < -camera_deadzone_x - camera_offset_x) {
            a_x = a_x + camera_deadzone_x - camera_offset_x;
            camera_x -= a_x >> 5;
        } else if (plat_camera_follow & 2 && a_x > camera_deadzone_x - camera_offset_x) {
            a_x = a_x - camera_deadzone_x - camera_offset_x;
            camera_x -= a_x >> 5;
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
