#ifndef STATE_PLATFORM_H
#define STATE_PLATFORM_H

#include <gb/gb.h>

void platform_init();
void platform_update();
void acceleration(BYTE dir) BANKED;
void deceleration() BANKED;
void basic_anim() BANKED;
void dash_check() BANKED;
void jump_init() BANKED;
void ladder_check() BANKED;
void wall_check() BANKED;
void basic_x_col() BANKED;
void basic_y_col(UBYTE drop_press) BANKED;
void ground_reset() NONBANKED;



extern WORD pl_vel_x;
extern WORD pl_vel_y;
extern WORD plat_min_vel;
extern WORD plat_walk_vel;
extern WORD plat_run_vel;
extern WORD plat_climb_vel;
extern WORD plat_walk_acc;
extern WORD plat_run_acc;
extern WORD plat_dec;
extern WORD plat_jump_vel;
extern WORD plat_grav;
extern WORD plat_hold_grav;
extern WORD plat_max_fall_vel;

#endif
