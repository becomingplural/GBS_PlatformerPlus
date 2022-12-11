#ifndef CAMERA_H
#define CAMERA_H

#include <gb/gb.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define SCREEN_WIDTH_HALF 80
#define SCREEN_HEIGHT_HALF 72

#define CAMERA_LOCK_FLAG 0x03
#define CAMERA_LOCK_X_FLAG 0x01
#define CAMERA_LOCK_Y_FLAG 0x02
#define CAMERA_UNLOCKED 0x00

extern INT16 camera_x;
extern INT16 camera_y;
extern BYTE camera_offset_x;
extern BYTE camera_offset_y;
extern UBYTE camera_deadzone_x;
extern UBYTE camera_deadzone_y;
extern UBYTE camera_settings;
extern UBYTE plat_camera_follow;
extern UBYTE plat_camera_lead;
extern UBYTE plat_camera_catchup;
extern WORD pl_vel_x;
//plat_camera_follow stores info as 4 bits: Up, Down, Left, Right


void camera_init() BANKED;
void camera_reset() BANKED;
void camera_update() NONBANKED;

#endif