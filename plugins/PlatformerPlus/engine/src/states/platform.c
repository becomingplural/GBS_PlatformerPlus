/*
Future notes on things to do:
- Limit air dashes before touching the ground
- Add an option for wall jump that only allows alternating walls.
- Currently, dashing through walls checks the potential end point, and if it isn't clear then it continues with the normal dash routine. 
    The result is that there could be a valid landing point across a wall, but the player is just a little too close for it to register. 
    I could create a 'look-back' loop that runs through the intervening tiles until it finds an empty landing spot.
- The bounce event is a funny one, because it can have the player going up without being in the jump state. I should perhaps add some error catching stuff for such situations
- Check for a bug with sticking to screen edges anytime the player collision box has a negative X value.

TARGETS for Optimization
- Is there any way to simplify the number of if branches with the solid actors?
- It's inellegant that the dash check requires me to check again later if it succeeded or not. Can I reorganize this somehow?

THINGS TO WATCH
- Note, the way I've written the cascading state switch logic, if a player hits jump, they will not do a ladder check the same frame. That seems fine, but keep an eye on it.
- I recently made it so that solid actors only reset the player's velocity if they aren't pressing a direction. Keep an eye on this.

NOTES on GBStudio Quirks
- 256 velocities per position, 16 'positions' per pixel, 8 pixels per tile
- Player bounds: for an ordinary 16x16 sprite, bounds.left starts at 0 and bounds.right starts at 16. If it's smaller, bounds.left is POSITIVE
- For bounds.top, however, Y starts counting from the middle of a sprite. bounds.top is negative and bounds.bottom is positive 

GENERAL STRUCTURE OF THIS FILE
Init()
    Tweak a few fields so they don't overflow variables
    Normalize some fields so that they are applied over multiple frames
    Initialize variables
UPDATE()
    A.Input checks (double-tap dash, float, drop-through)
    B.STATE MACHINE
        States: Falling, Ground, Jumping, Dashing, Climbing a Ladder, Wall Sliding
        Each one has the following structure
            1. Calculate change in Horizontal Movement
            2. Calculate change in Vertical Movement
            3. Check for collisions with Horizontal Movement
            4. Check for collisions with Vertical Movement
            5. Update animations
            6. Check for changes to the current state
    C.Check for collisions with triggers and actors
    D.Update counters

For some of the sub-stages, common versions are carved off into seperate functions. 

*/
#pragma bank 3

#include "data/states_defines.h"
#include "states/platform.h"

#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "data_manager.h"
#include "game_time.h"
#include "input.h"
#include "math.h"
#include "scroll.h"
#include "trigger.h"
#include "vm.h"

#ifndef INPUT_PLATFORM_JUMP
#define INPUT_PLATFORM_JUMP        INPUT_A
#endif
#ifndef INPUT_PLATFORM_RUN
#define INPUT_PLATFORM_RUN         INPUT_B
#endif
#ifndef INPUT_PLATFORM_INTERACT
#define INPUT_PLATFORM_INTERACT    INPUT_A
#endif
#ifndef PLATFORM_CAMERA_DEADZONE_X
#define PLATFORM_CAMERA_DEADZONE_X 4
#endif
#ifndef PLATFORM_CAMERA_DEADZONE_Y
#define PLATFORM_CAMERA_DEADZONE_Y 16
#endif


//DEFAULT ENGINE VARIABLES
WORD plat_min_vel;
WORD plat_walk_vel;
WORD plat_run_vel;
WORD plat_climb_vel;
WORD plat_walk_acc;
WORD plat_run_acc;
WORD plat_dec;
WORD plat_jump_vel;
WORD plat_grav;
WORD plat_hold_grav;
WORD plat_max_fall_vel;

//PLATFORMER PLUS ENGINE VARIABLES
//All engine fields are prefixed with plat_
UBYTE plat_drop_through;    //Drop-through control
UBYTE plat_mp_group;        //Collision group for platform actors
UBYTE plat_solid_group;     //Collision group for solid actors
WORD plat_jump_min;         //Jump amount applied on the first frame of jumping
UBYTE plat_hold_jump_max;   //Maximum number for frames for continuous input
UBYTE plat_extra_jumps;     //Number of jumps while in the air
WORD plat_jump_reduction;   //Reduce height each double jump
UBYTE plat_coyote_max;      //Coyote Time maximum frames
UBYTE plat_buffer_max;      //Jump Buffer maximum frames
UBYTE plat_wall_jump_max;   //Number of wall jumps in a row
UBYTE plat_wall_slide;      //Enables/Disables wall sliding
WORD plat_wall_grav;        //Gravity while clinging to the wall
WORD plat_wall_kick;        //Horizontal force for pushing off the wall
UBYTE plat_float_input;     //Input type for float (hold up or hold jump)
WORD plat_float_grav;       //Speed of fall descent while floating
UBYTE plat_air_control;     //Enables/Disables air control
UBYTE plat_turn_control;    //Controls the amount of slippage when the player turns while running.
WORD plat_air_dec;          // air deceleration rate
UBYTE plat_run_type;        //Chooses type of acceleration for jumping
WORD plat_turn_acc;         //Speed with which a character turns
UBYTE plat_run_boost;        //Additional jump height based on player horizontal speed
UBYTE plat_dash;            //Choice of input for dashing: double-tap, interact, or down and interact
UBYTE plat_dash_style;      //Ground, air, or both
UBYTE plat_dash_momentum;   //Applies horizontal momentum or vertical momentum, neither or both
UBYTE plat_dash_through;    //Choose if the player can dash through actors, triggers, and walls
WORD plat_dash_dist;        //Distance of the dash
UBYTE plat_dash_frames;     //Number of frames for dashing
UBYTE plat_dash_ready_max;  //Time before the player can dash again

enum pStates {              //Datatype for tracking states
    FALL_INIT = 0,
    FALL_STATE,
    GROUND_INIT,
    GROUND_STATE,
    JUMP_INIT,
    JUMP_STATE,
    DASH_INIT,
    DASH_STATE,
    LADDER_INIT,
    LADDER_STATE,
    WALL_INIT,
    WALL_STATE,
    KNOCKBACK_INIT,
    KNOCKBACK_STATE,
    BLANK_INIT,
    BLANK_STATE
}; 
enum pStates plat_state;    //Current platformer state
UBYTE nocontrol_h;          //Turns off horizontal input, currently only for wall jumping
UBYTE nocollide;            //Turns off vertical collisions, currently only for dropping through platforms

//COUNTER variables
UBYTE ct_val;               //Coyote Time Variable
UBYTE jb_val;               //Jump Buffer Variable
UBYTE wc_val;               //Wall Coyote Time Variable
UBYTE hold_jump_val;        //Jump input hold variable
UBYTE dj_val;               //Current double jump
UBYTE wj_val;               //Current wall jump

//WALL variables 
BYTE last_wall;             //tracks the last wall the player touched
BYTE col;                   //tracks if there is a block left or right

//DASH VARIABLES
UBYTE dash_ready_val;       //tracks the current amount before the dash is ready
WORD dash_dist;             //Takes overall dash distance and holds the amount per-frame
UBYTE dash_currentframe;    //Tracks the current frame of the overall dash
BYTE tap_val;               //Number of frames since the last time left or right button was tapped
UBYTE dash_end_clear;       //Used to store the result of whether the end-position of a dash is empty

//COLLISION VARS
actor_t *last_actor;        //The last actor the player hit, and that they were attached to
UBYTE actor_attached;       //Keeps track of whether the player is currently on an actor and inheriting its movement
WORD mp_last_x;             //Keeps track of the pos.x of the attached actor from the previous frame
WORD mp_last_y;             //Keeps track of the pos.y of the attached actor from the previous frame
WORD deltaX;                //Change in X velocity this frame. Necessary to add precise positional data from platforms before collisions are calculated
WORD deltaY;                //Change in y velocity this frame
WORD actorColX;             //Offset from colliding with a solid actor in the previous frame from the side
WORD actorColY;             //As above but for hitting the roof

//JUMPING VARIABLES
WORD jump_reduction_val;    //Holds a temporary jump velocity reduction
WORD jump_per_frame;        //Holds a jump amount that has been normalized over the number of jump frames
WORD jump_reduction;        //Holds the reduction amount that has been normalized over the number of jump frames
WORD boost_val;

//WALKING AND RUNNING VARIABLES
WORD pl_vel_x;              //Tracks the player's x-velocity between frames
WORD pl_vel_y;              //Tracks the player's y-velocity between frames

//VARIABLES FOR EVENT PLUGINS
UBYTE grounded;             //Variable to keep compatability with other plugins that use the older 'grounded' check
BYTE run_stage;             //Tracks the stage of running based on the run type
UBYTE jump_type;            //Tracks the type of jumping, from the ground, in the air, or off the wall


void platform_init() BANKED {
    //Make sure jumping doesn't overflow variables
    //First, check for jumping based on Frames and Initial Jump Min
    while (32000 - (plat_jump_vel/MIN(15,plat_hold_jump_max)) - plat_jump_min < 0){
        plat_hold_jump_max += 1;
    }

    //This ensures that, by itself, the plat run boost active on any single frame cannot overflow a WORD.
    //It is complemented by another check in the jump itself that works with the actual velocity. 
    if (plat_run_boost != 0){
        while((32000/plat_run_boost) < ((plat_run_vel>>8)/plat_hold_jump_max)){
            plat_run_boost--;
        }
    }

    //Normalize variables by number of frames
    jump_per_frame = plat_jump_vel / MIN(15, plat_hold_jump_max);   //jump force applied per frame in the JUMP_STATE
    jump_reduction = plat_jump_reduction / plat_hold_jump_max;      //Amount to reduce subequent jumps per frame in JUMP_STATE
    dash_dist = plat_dash_dist/plat_dash_frames;                    //Dash distance per frame in the DASH_STATE
    boost_val = plat_run_boost/plat_hold_jump_max;                  //Vertical boost from horizontal speed per frame in JUMP STATE

    //Initialize State
    plat_state = FALL_INIT;
    actor_attached = FALSE;
    grounded = FALSE;
    run_stage = 0;
    nocontrol_h = 0;
    nocollide = 0;
    actorColX = 0;
    actorColY = 0;
    if (PLAYER.dir == DIR_UP || PLAYER.dir == DIR_DOWN) {
        PLAYER.dir = DIR_RIGHT;
    }

    //Initialize other vars
    camera_offset_x = 0;
    camera_offset_y = 0;
    camera_deadzone_x = PLATFORM_CAMERA_DEADZONE_X;
    camera_deadzone_y = PLATFORM_CAMERA_DEADZONE_Y;
    game_time = 0;
    pl_vel_x = 0;
    pl_vel_y = plat_grav << 2;
    last_wall = 0;
    col = 0;
    hold_jump_val = plat_hold_jump_max;
    dj_val = 0;
    wj_val = plat_wall_jump_max;
    dash_end_clear = FALSE;
    jump_type = 0;
}

void platform_update() BANKED {
    //INITIALIZE VARS
    actor_t *hit_actor;
    UBYTE tile_start, tile_end;     //I'm not sure why I can't localize these into the states that use them. Seems to be a leak. But maybe it's a switch issue.
    WORD temp_y = 0;
    deltaX = 0;                     //DeltaX/DeltaY measure change in distance per frame. Reset here.
    deltaY = 0;

    //A. INPUT CHECK=================================================================================================
    //Dash Input Check
    UBYTE dash_press = FALSE;
    switch(plat_dash){
        case 1:
            //Interact Dash
            if (INPUT_PRESSED(INPUT_PLATFORM_INTERACT)){
                dash_press = TRUE;
            }
        break;
        case 2:
            //Double-Tap Dash
            if (INPUT_PRESSED(INPUT_LEFT)){
                if(tap_val < 0){
                    dash_press = TRUE;
                } else{
                    tap_val = -15;
                }
            } else if (INPUT_PRESSED(INPUT_RIGHT)){
                if(tap_val > 0){
                    dash_press = TRUE;
                } else{
                    tap_val = 15;
                }
            }
        break;
        case 3:
            //Down and Interact (need to check both orders)
            if ((INPUT_PRESSED(INPUT_DOWN) && INPUT_PLATFORM_JUMP) || (INPUT_DOWN && INPUT_PRESSED(INPUT_PLATFORM_JUMP))){
                dash_press = TRUE;
            }
        break;
    }

    //Drop Through Press
    UBYTE drop_press = FALSE;
    if ((plat_drop_through == 1 && INPUT_DOWN ) || (plat_drop_through == 2 && INPUT_DOWN && INPUT_PLATFORM_JUMP)){
        drop_press = TRUE;
    }
    //FLOAT INPUT
    UBYTE float_press = FALSE;
    if ((plat_float_input == 1 && INPUT_PLATFORM_JUMP) || (plat_float_input == 2 && INPUT_UP)){
        float_press = TRUE;
    }

    // B. STATE MACHINE==================================================================================================
    // Change to Switch Statement
    switch(plat_state){
        case LADDER_INIT:
            plat_state = LADDER_STATE;
            jump_type = 0;
        case LADDER_STATE:{
            //For positioning the player in the middle of the ladder
            UBYTE p_half_width = (PLAYER.bounds.right - PLAYER.bounds.left) >> 1;
            UBYTE tile_x_mid = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left + p_half_width) >> 3; 
            pl_vel_y = 0;
            if (INPUT_UP) {
                // Climb laddder
                UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
                //Check if the tile above the player is a ladder tile. If so add ladder velocity
                if (tile_at(tile_x_mid, tile_y) & TILE_PROP_LADDER) {
                    pl_vel_y = -plat_climb_vel;
                }
            } else if (INPUT_DOWN) {
                // Descend ladder
                UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom + 1) >> 3;
                if (tile_at(tile_x_mid, tile_y) & TILE_PROP_LADDER) {
                    pl_vel_y = plat_climb_vel;
                }
            } else if (INPUT_LEFT) {
                plat_state = FALL_INIT; //Assume we're going to leave the ladder state, 
                // Check if able to leave ladder on left
                tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
                tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
                while (tile_start != tile_end) {
                    if (tile_at(tile_x_mid - 1, tile_start) & COLLISION_RIGHT) {
                        plat_state = LADDER_STATE; //If there is a wall, stay on the ladder.
                        break;
                    }
                    tile_start++;
                }            
            } else if (INPUT_RIGHT) {
                plat_state = FALL_INIT;
                // Check if able to leave ladder on right
                tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
                tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
                while (tile_start != tile_end) {
                    if (tile_at(tile_x_mid + 1, tile_start) & COLLISION_LEFT) {
                        plat_state = LADDER_STATE;
                        break;
                    }
                    tile_start++;
                }
            }
            PLAYER.pos.y += (pl_vel_y >> 8);

            //Animation----------------------------------------------------------------------------------------------------
            actor_set_anim(&PLAYER, ANIM_CLIMB);
            if (pl_vel_y == 0) {
                actor_stop_anim(&PLAYER);
            }

            //State Change-------------------------------------------------------------------------------------------------
            //Collision logic provides options for exiting to Neutral

            //Above is the default GBStudio setup. However it seems worth adding a jump-from-ladder option, at the very least to drop down.
            if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
                plat_state = FALL_INIT;
            }


        }
        break;
    //================================================================================================================
        case DASH_INIT:
            plat_state = DASH_STATE;
            jump_type = 0;
            run_stage = 0;
        case DASH_STATE: {
            //Movement & Collision Combined----------------------------------------------------------------------------------
            //Dashing uses much of the basic collision code. Comments here focus on the differences.
            UBYTE tile_current; //For tracking collisions across longer distances
            tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
            tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;        
            col = 0;

            //Right Dash Movement & Collision
            if (PLAYER.dir == DIR_RIGHT){
                //Get tile x-coord of player position
                tile_current = ((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3;
                //Get tile x-coord of final position
                UWORD new_x = PLAYER.pos.x + (dash_dist);
                UBYTE tile_x = (((new_x >> 4) + PLAYER.bounds.right) >> 3) + 1;
                //check each space from the start of the dash until the end of the dash.
                //in the future, I should build a reversed version of this section for dashing through walls.
                //However, it's not quite as simple as reversing the direction of the check. The loops need to store the player's width and only return when there are enough spaces in a row
                while (tile_current != tile_x){
                    //CHECK TOP AND BOTTOM
                    while (tile_start != tile_end) {
                        //Check for Collisions (if the player collides with walls)
                        if(plat_dash_through != 3 || dash_end_clear == FALSE){                    
                            if (tile_at(tile_current, tile_start) & COLLISION_LEFT) {
                                //The landing space is the tile we collided on, but one to the left
                                new_x = ((((tile_current) << 3) - PLAYER.bounds.right) << 4) -1;
                                col = 1;
                                last_wall = 1;
                                wc_val = plat_coyote_max;
                                dash_currentframe == 0;
                                goto endRcol;
                            }   
                        }
                        //Check for Triggers at each step. If there is a trigger stop the dash (but don't run the trigger yet).
                        if (plat_dash_through < 2){
                            if (trigger_at_tile(tile_current, tile_start) != NO_TRIGGER_COLLISON) {
                                new_x = ((((tile_current+1) << 3) - PLAYER.bounds.right) << 4);
                            }
                        }
                        tile_start++;
                    }
                    tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top) >> 3);
                    tile_current += 1;
                }
                endRcol: 
                if(plat_dash_momentum == 1 || plat_dash_momentum == 3){           
                    //Dashes don't actually use velocity, so we will simulate the momentum by adding the full run speed. 
                    pl_vel_x = plat_run_vel;
                } else{
                    pl_vel_x = 0;
                }
                PLAYER.pos.x = MIN((image_width - 16) << 4, new_x);
            }

            //Left Dash Movement & Collision
            else  if (PLAYER.dir == DIR_LEFT){
                //Get tile x-coord of player position
                tile_current = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left) >> 3;
                //Get tile x-coord of final position
                WORD new_x = PLAYER.pos.x - (dash_dist);
                UBYTE tile_x = (((new_x >> 4) + PLAYER.bounds.left) >> 3)-1;
                //CHECK EACH SPACE FROM START TO END
                while (tile_current != tile_x){
                    //CHECK TOP AND BOTTOM
                    while (tile_start != tile_end) {   
                        //check for walls
                        if(plat_dash_through != 3 || dash_end_clear == FALSE){  //If you collide with walls
                            if (tile_at(tile_current, tile_start) & COLLISION_RIGHT) {
                                new_x = ((((tile_current + 1) << 3) - PLAYER.bounds.left) << 4)+1;
                                col = -1;
                                last_wall = -1;
                                dash_currentframe == 0;
                                wc_val = plat_coyote_max;
                                goto endLcol;
                            }
                        }
                        //Check for triggers
                        if (plat_dash_through  < 2){
                            if (trigger_at_tile(tile_current, tile_start) != NO_TRIGGER_COLLISON) {
                                new_x = ((((tile_current - 1) << 3) - PLAYER.bounds.left) << 4);
                                goto endLcol;
                            }
                        }  
                        tile_start++;
                    }
                    tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top) >> 3);
                    tile_current -= 1;
                }
                endLcol: 
                if(plat_dash_momentum == 1 || plat_dash_momentum == 3){            
                    pl_vel_x = -plat_run_vel;
                } else{
                    pl_vel_x = 0;
                }
                PLAYER.pos.x = MAX(0, new_x);
            }

            //Vertical Movement & Collision-------------------------------------------------------------------------
            if(plat_dash_momentum >= 2){
                //If we're using vertical momentum, add gravity as normal (otherwise, vel_y = 0)
                pl_vel_y += plat_hold_grav;

                //Add Jump force
                if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
                    //Coyote Time (CT) functions here as a proxy for being grounded. 
                    if (ct_val != 0){
                        actor_attached = FALSE;
                        pl_vel_y = -(plat_jump_min + (plat_jump_vel/2));
                        jb_val = 0;
                        ct_val = 0;
                        jump_type = 1;
                    } else if (dj_val != 0){
                    //If the player is in the air, and can double jump
                        dj_val -= 1;
                        jump_reduction_val += jump_reduction;
                        actor_attached = FALSE;
                        //We can't switch states for jump frames, so approximate the height. Engine val limits ensure this doesn't overflow.
                        pl_vel_y = -(plat_jump_min + (plat_jump_vel/2));    
                        jb_val = 0;
                        ct_val = 0;
                        jump_type = 2;
                    }
                } 

                //Vertical Collisions
                temp_y = PLAYER.pos.y;    
                deltaY = pl_vel_y >> 8;
                deltaY += actorColY;
                deltaY = CLAMP(deltaY, -127, 127);
                tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
                tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
                if (deltaY > 0) {

                //Moving Downward
                    WORD new_y = PLAYER.pos.y + deltaY;
                    UBYTE tile_yz = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;
                    while (tile_start != tile_end) {
                        if (tile_at(tile_start, tile_yz) & COLLISION_TOP) {                    
                            //Land on Floor
                            new_y = ((((tile_yz) << 3) - PLAYER.bounds.bottom) << 4) - 1;
                            actor_attached = FALSE; //Detach when MP moves through a solid tile.                                   
                            pl_vel_y = 0;
                            break;
                        }
                        tile_start++;
                    }
                    PLAYER.pos.y = new_y;
                } else if (deltaY < 0) {

                    //Moving Upward
                    WORD new_y = PLAYER.pos.y + deltaY;
                    UBYTE tile_yz = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
                    while (tile_start != tile_end) {
                        if (tile_at(tile_start, tile_yz) & COLLISION_BOTTOM) {
                            new_y = ((((UBYTE)(tile_yz + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                            pl_vel_y = 0;
                            break;
                        }
                        tile_start++;
                    }
                    PLAYER.pos.y = new_y;
                }
                // Clamp Y Velocity
                pl_vel_y = CLAMP(pl_vel_y,-plat_max_fall_vel, plat_max_fall_vel);
            } else{
                temp_y = PLAYER.pos.y;  
            }
            //ANIMATION-------------------------------------------------------------------------------------------------------
            //Currently this animation uses the 'jump' animation is it's default. 
            basic_anim();

            //STATE CHANGE: No exits above.------------------------------------------------------------------------------------
            //DASH -> NEUTRAL Check
            //Colliding with a wall sets the currentframe to 0 above.
            if (dash_currentframe == 0){
                plat_state = FALL_INIT;
            }
        }
        break;  
    //================================================================================================================
        case GROUND_INIT:
            plat_state = GROUND_STATE;
            jump_type = 0;
            ct_val = plat_coyote_max; 
            dj_val = plat_extra_jumps; 
            wj_val = plat_wall_jump_max;
            jump_reduction_val = 0;
        case GROUND_STATE:{
            //Horizontal Motion---------------------------------------------------------------------------------------------
            if (INPUT_LEFT) {
                acceleration(-1);
            } else if (INPUT_RIGHT) {
                acceleration(1);
            } else {
                deceleration();
                deltaX = pl_vel_x;
            }

            //Add X motion from moving platforms
            //Transform velocity into positional data, to keep the precision of the platform's movement
            deltaX = deltaX >> 8;
            if (actor_attached){
                //If the platform has been disabled, detach the player
                if(last_actor->disabled == TRUE){
                    plat_state = FALL_INIT;
                    actor_attached = FALSE;
                //If the player is off the platform to the right, detach from the platform
                } else if (PLAYER.pos.x + (PLAYER.bounds.left << 4) > last_actor->pos.x + (last_actor->bounds.right<< 4)) {
                    plat_state = FALL_INIT;
                    actor_attached = FALSE;
                //If the player is off the platform to the left, detach
                } else if (PLAYER.pos.x + (PLAYER.bounds.right << 4) < last_actor->pos.x + (last_actor->bounds.left << 4)){
                    plat_state = FALL_INIT;
                    actor_attached = FALSE;
                } else{
                //Otherwise, add any change in movement from platform
                    deltaX += (last_actor->pos.x - mp_last_x);
                    mp_last_x = last_actor->pos.x;
                }
            }
            deltaX += actorColX;

            // Vertical Motion-------------------------------------------------------------------------------------------------
            if (actor_attached){
                //If we're on a platform, zero out any other motion from gravity or other sources
                pl_vel_y = 0;
            } else if (nocollide != 0){
                //If we're dropping through a platform
                pl_vel_y = 7000; //magic number, rough minimum for actually having the player descend through a platform
            } else {
                //Normal gravity
                pl_vel_y += plat_grav;
            }

            // Add Y motion from moving platforms
            deltaY = pl_vel_y >> 8;
            deltaY += actorColY; //Add any change from collisions with solid platforms (ie. not what we're standing on)
            if (actor_attached){
                //Add any change from the platform we're standing on
                deltaY += last_actor->pos.y - mp_last_y;

                //We're setting these to the platform's position, rather than the actor so that if something causes the player to
                //detach (like hitting the roof), they won't automatically get re-attached in the subsequent actor collision step.
                mp_last_y = last_actor->pos.y;
                temp_y = last_actor->pos.y;
            }
            else{
                temp_y = PLAYER.pos.y;
            }
      
            //Collision-----------------------------------------------------------------------------------------------------
            //Horizontal Collision Checks
            basic_x_col();

            //Vertical Collision Checks
            basic_y_col(drop_press);

            //ANIMATION---------------------------------------------------------------------------------------------------
            //Button direction overrides velocity, for slippery run reasons
            if (INPUT_LEFT){
                actor_set_dir(&PLAYER, DIR_LEFT, TRUE);
            } else if (INPUT_RIGHT){
                actor_set_dir(&PLAYER, DIR_RIGHT, TRUE);
            } else if (pl_vel_x < 0) {
                actor_set_dir(&PLAYER, DIR_LEFT, TRUE);
            } else if (pl_vel_x > 0) {
                actor_set_dir(&PLAYER, DIR_RIGHT, TRUE);
            } else {
                actor_set_anim_idle(&PLAYER);
            }

            //STATE CHANGE: Above, basic_y_col can shift to FALL_STATE.--------------------------------------------------
            //GROUND -> DASH Check
            if((plat_state == GROUND_STATE || plat_state == GROUND_INIT) && dash_press){
                if(plat_dash_style != 1){
                    dash_check();
                }
            }
            //GROUND -> JUMP Check
            if (plat_state != DASH_STATE && nocollide == 0){    //If we started dashing, don't do other checks.
                if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
                    //Standard Jump
                    jump_type = 1;
                    jump_init();
                } else if (jb_val !=0){
                    //Jump Buffered Jump
                    jump_type = 1;
                    jump_init();
                }
                else{
            //GROUND -> LADDER Check
                    ladder_check();
                }
            }
        }
        break;
    //================================================================================================================
        case JUMP_INIT:
            plat_state = JUMP_STATE;
        case JUMP_STATE: {
            //Horizontal Movement-----------------------------------------------------------------------------------------
            if (nocontrol_h != 0 || plat_air_control == 0){
                //If the player doesn't have control of their horizontal movement
                deltaX = pl_vel_x;
            } else if (INPUT_LEFT) {
                //Move left
                acceleration(-1);
            } else if (INPUT_RIGHT) {
                //Move right
                acceleration(1);
            } else {
                //No input, use friction to decelerate
                deceleration();
                deltaX = pl_vel_x;
            }

            //Vertical Movement-------------------------------------------------------------------------------------------
            //Add jump force during each jump frame
            if (hold_jump_val !=0 && INPUT_PLATFORM_JUMP){
                //Add the boost per frame amount.
                pl_vel_y -= jump_per_frame;
                //Reduce subsequent jump amounts (for double jumps)
                if (plat_jump_vel >= jump_reduction_val){
                    pl_vel_y += jump_reduction_val;
                } else {
                    //When reducing that value, zero out if it's negative
                    pl_vel_y = 0;
                }
                //Add jump boost from horizontal movement
                WORD tempBoost = (pl_vel_x >> 8) * boost_val;
                //Take the positive value of x-vel
                tempBoost = MAX(tempBoost, -tempBoost);
                //This is a test to see if the results will overflow pl_vel_y. Note, pl_vel_y is negative here.
                if (tempBoost > 32767 + pl_vel_y){
                    pl_vel_y = -32767;
                }
                else{
                    pl_vel_y += -tempBoost;
                }
                hold_jump_val -=1;
            } else if (INPUT_PLATFORM_JUMP && pl_vel_y < 0){
                //After the jump frames end, use the reduced gravity
                ct_val = 0; //Coyote time gets reset here, rather than immediately, so that dash-jumping works properly
                pl_vel_y += plat_hold_grav;
            } else {
                //Shift out of jumping if the player stops pressing OR if they run out of jump frames
                ct_val = 0;
                plat_state = FALL_INIT;
            }

            //Collision Checks ---------------------------------------------------------------------------------------
            //Horizontal Collision
            deltaX = deltaX >> 8;               //Convert velocity to positional scale in order to incorporate the changes from other actors
            deltaX += actorColX;
            basic_x_col();

            //Vertical Collision
            //Because the player exits this state if they are moving downwards, we do not need that collision
            deltaY = pl_vel_y >> 8;
            deltaY += actorColY;
            deltaY = CLAMP(deltaY,-127,127);    //128 positional units is one tile. Moving further than this breaks the collision detection.
            temp_y = PLAYER.pos.y;    
            tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
            tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
            if (deltaY < 0) {
                //Moving Upward                 
                WORD new_y = PLAYER.pos.y + deltaY;
                UBYTE tile_y = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
                while (tile_start != tile_end) {
                    if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {
                        new_y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                        pl_vel_y = 0;
                        ct_val = 0;
                        plat_state = FALL_INIT;
                        break;
                    }
                    tile_start++;
                }
                PLAYER.pos.y = new_y;
            }
            // Clamp Y Velocity
            pl_vel_y = CLAMP(pl_vel_y,-plat_max_fall_vel, plat_max_fall_vel);

            //ANIMATION---------------------------------------------------------------------------------------------------
            basic_anim();

            //STATE CHANGE------------------------------------------------------------------------------------------------
            //Above: JUMP-> NEUTRAL when a) player starts descending, b) player hits roof, c) player stops pressing, d)jump frames run out.
            //JUMP -> WALL check
            wall_check();
            //JUMP -> DASH check
            if(dash_press){
                if(plat_dash_style != 0 || ct_val != 0){
                    dash_check();
                }
            }
            //JUMP -> LADDER check
            if (plat_state != DASH_STATE){
                ladder_check();
            }
        }
        break;
    //================================================================================================================
        case WALL_INIT:
            plat_state = WALL_STATE;
            jump_type = 0;
            run_stage = 0;
        case WALL_STATE:{
            //Horizontal Movement----------------------------------------------------------------------------------------
            if (INPUT_LEFT) {
                acceleration(-1);
            } else if (INPUT_RIGHT) {
                acceleration(1);
            } else {
                deceleration();
                deltaX = pl_vel_x;
            }

            //Vertical Movement------------------------------------------------------------------------------------------
            //WALL SLIDE
            if (nocollide != 0){
                pl_vel_y += 7000; //magic number, rough minimum for actually having the player descend through a platform
            } else if (pl_vel_y < 0){
                //If the player is still ascending, don't apply wall-gravity
                pl_vel_y += plat_grav;
            } else if (plat_wall_slide) {
                //If the toggle is on, use wall gravity
                pl_vel_y = plat_wall_grav;
            } else{
                //Otherwise use regular gravity
                pl_vel_y += plat_grav;
            }

            //Collision--------------------------------------------------------------------------------------------------
            //Horizontal Collision Checks
            deltaX = deltaX >> 8;
            deltaX += actorColX;
            basic_x_col();

            //Vertical Collision Checks
            deltaY = pl_vel_y >> 8;
            deltaY += actorColY;
            temp_y = PLAYER.pos.y;    
            basic_y_col(drop_press);

            //ANIMATION---------------------------------------------------------------------------------------------------
            //Face away from walls
            
            if (col == 1){
                  actor_set_dir(&PLAYER, DIR_LEFT, TRUE);
            } else if (col == -1){
                actor_set_dir(&PLAYER, DIR_RIGHT, TRUE);
            }


            //STATE CHANGE------------------------------------------------------------------------------------------------
            //Above, basic_y_col can cause WALL -> GROUNDED.
            //Exit state as baseline
            wall_check();
            //WALL -> DASH Check
            if(dash_press){
                if (plat_state == WALL_STATE && plat_dash_style != 0){
                    dash_check();
                }
                else if (plat_state == GROUND_INIT && plat_dash_style != 1){
                    dash_check();
                }
            }
            //WALL -> JUMP Check
            if (plat_state != DASH_STATE){
                if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
                    //Wall Jump
                    if(wj_val != 0){
                        wj_val -= 1;
                        nocontrol_h = 5;
                        pl_vel_x += (plat_wall_kick + plat_walk_vel)*-last_wall;
                        jump_type = 3;
                        jump_init();

                    }
                } else {
            //WALL -> LADDER Check
                    ladder_check();
                }
            }
        }
        break;
    //================================================================================================================
        case FALL_INIT:
            plat_state = FALL_STATE;
        case FALL_STATE: {
            jump_type = 0;  //Keep this here, rather than in init, so that we can easily track float as a jump type  
            //Horizontal Movement----------------------------------------------------------------------------------------
            if (nocontrol_h != 0){
                //No horizontal input
                deltaX = pl_vel_x;
            } else if (plat_air_control == 0){
                deltaX = pl_vel_x;
                //No accel or decel in mid-air
            } else if (INPUT_LEFT) {
                acceleration(-1);
            } else if (INPUT_RIGHT) {
                acceleration(1);
            } else {
                deceleration();
                deltaX = pl_vel_x;
            }
        
            //Vertical Movement--------------------------------------------------------------------------------------------
            if (nocollide != 0){
                pl_vel_y = 7000; //magic number, rough minimum for actually having the player descend through a platform
            } else if (INPUT_PLATFORM_JUMP && pl_vel_y < 0) {
                //Gravity while holding jump
                pl_vel_y += plat_hold_grav;
            } else if (float_press && pl_vel_y > 0){
                pl_vel_y = plat_float_grav;
                jump_type = 4;
            } else {
                //Normal gravity
                pl_vel_y += plat_grav;
            }
        
            //Collision ---------------------------------------------------------------------------------------------------
            //Horizontal Collision Checks
            deltaX = deltaX >> 8;
            deltaX += actorColX;
            basic_x_col();

            //Vertical Collision Checks
            deltaY = pl_vel_y >> 8;
            deltaY += actorColY;
            temp_y = PLAYER.pos.y;    
            basic_y_col(drop_press); 
            
            //ANIMATION--------------------------------------------------------------------------------------------------
            basic_anim();

            //STATE CHANGE------------------------------------------------------------------------------------------------
            //Above: FALL -> GROUND in basic_y_col()
            //FALL -> WALL check
            wall_check();
            //FALL -> DASH check
            if(dash_press){
                if ((plat_state == FALL_STATE || plat_state == FALL_INIT) && plat_dash_style != 0){
                    dash_check();
                }
                else if (plat_state == GROUND_INIT && plat_dash_style != 1){
                    dash_check();
                }
            }
            //FALL -> JUMP check (we could have started dashing, in which case ignore jump code)
            if(plat_state != DASH_STATE){
                if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
                    //Wall Jump
                    if(wc_val != 0){
                        if(wj_val != 0){
                            jump_type = 3;
                            wj_val -= 1;
                            nocontrol_h = 5;
                            pl_vel_x += (plat_wall_kick + plat_walk_vel)*-last_wall;
                            jump_init();
                        }
                    }
                    if (ct_val != 0){
                    //Coyote Time Jump
                        jump_type = 1;
                        jump_init();
                    } else if (dj_val != 0){
                    //Double Jump
                        jump_type = 2;
                        if (dj_val != 255){
                            dj_val -= 1;
                        }
                        jump_reduction_val += jump_reduction;
                        jump_init();
                    } else {
                    // Setting the Jump Buffer when jump is pressed while not on the ground
                    jb_val = plat_buffer_max; 
                    }
                } else{
            //NEUTRAL -> LADDER check
                    ladder_check();
                } 
            }
        }
        break;
    //================================================================================================================
        case KNOCKBACK_INIT:
        run_stage = 0;
        jump_type = 0;
        plat_state = KNOCKBACK_STATE;
        case KNOCKBACK_STATE: {
            //Horizontal Movement----------------------------------------------------------------------------------------
            if (pl_vel_x < 0) {
                    pl_vel_x += plat_dec;
                    pl_vel_x = MIN(pl_vel_x, 0);
            } else if (pl_vel_x > 0) {
                    pl_vel_x -= plat_dec;
                    pl_vel_x = MAX(pl_vel_x, 0);
            }
            deltaX = pl_vel_x;
        
            //Vertical Movement--------------------------------------------------------------------------------------------
            //Normal gravity
            pl_vel_y += plat_grav;
        
            //Collision ---------------------------------------------------------------------------------------------------
            //Horizontal Collision Checks
            deltaX = deltaX >> 8;
            deltaX += actorColX;
            basic_x_col();

            //Vertical Collision Checks
            deltaY = pl_vel_y >> 8;
            deltaY += actorColY;
            temp_y = PLAYER.pos.y;    
            deltaY = CLAMP(deltaY, -127, 127);
            tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
            tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
            if (deltaY > 0) {
                //Moving Downward
                WORD new_y = PLAYER.pos.y + deltaY;
                UBYTE tile_y = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;
                while (tile_start != tile_end) {
                    if (tile_at(tile_start, tile_y) & COLLISION_TOP) {
                        //Land on Floor
                        new_y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;
                        actor_attached = FALSE; //Detach when MP moves through a solid tile.
                        pl_vel_y = 0;
                        PLAYER.pos.y = new_y;
                    }
                    tile_start++;
                }
                PLAYER.pos.y = new_y;
            } else if (deltaY < 0) {
                //Moving Upward
                WORD new_y = PLAYER.pos.y + deltaY;
                UBYTE tile_y = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
                while (tile_start != tile_end) {
                    if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {
                        new_y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                        pl_vel_y = 0;
                        //MP Test: Attempting stuff to stop the player from continuing upward
                        if(actor_attached){
                            actor_attached = FALSE;
                            new_y = last_actor->pos.y;
                        }
                        break;
                    }
                    tile_start++;
                }
                PLAYER.pos.y = new_y;
            }
            // Clamp Y Velocity
            pl_vel_y = CLAMP(pl_vel_y,-plat_max_fall_vel, plat_max_fall_vel);
            
            //ANIMATION--------------------------------------------------------------------------------------------------
            //No change

            //STATE CHANGE------------------------------------------------------------------------------------------------
            //None: only player driven events can break out of this state
        }
        break;
    //================================================================================================================
        case BLANK_INIT:
        plat_state = BLANK_STATE;
        pl_vel_x = 0;
        pl_vel_y = 0;
        run_stage = 0;
        jump_type = 0;
        case BLANK_STATE: {
            //Movement: None
            //Collision: None
            //Animation: No change
            //State change: none, only player driven events can break out of this state.

        }
        break;
    }


    // C. TRIGGER AND ACTOR COLLISIONS===============================================================================
    // Check for trigger collisions
    if(plat_state != DASH_STATE || plat_dash_through < 2){
        if (trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED)) {
            // Landed on a trigger
            return;
        }
    }

    //Don't hit actors while dashing
    actorColX = 0;  //These vars are used to track the offset from solid actors each frame. Reseting them here.
    actorColY = 0;

    if(plat_state != DASH_STATE || plat_dash_through == 0){
        //Actor Collisions
        hit_actor = actor_overlapping_player(FALSE);
        if (hit_actor != NULL && hit_actor->collision_group) {
            //Solid Actors
            if (hit_actor->collision_group == plat_solid_group){
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < (hit_actor->pos.y + (hit_actor->bounds.top << 4)) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;                        
                        plat_state = GROUND_INIT;
                        //PLAYER bounds top seems to be 0 and counting down...
                    } else if (temp_y + (PLAYER.bounds.top<<4) > hit_actor->pos.y + (hit_actor->bounds.bottom<<4)){
                        actorColY += (hit_actor->pos.y - PLAYER.pos.y) + ((-PLAYER.bounds.top + hit_actor->bounds.bottom)<<4) + 32;
                        pl_vel_y = plat_grav;
                        if(plat_state == JUMP_STATE){
                            plat_state = FALL_INIT;
                        }

                    } else if (PLAYER.pos.x < hit_actor->pos.x){
                        actorColX = (hit_actor->pos.x - PLAYER.pos.x) - ((PLAYER.bounds.right + -hit_actor->bounds.left)<<4);
                        if(!INPUT_RIGHT){
                            pl_vel_x = 0;
                        }
                        if(plat_state == DASH_STATE){
                            plat_state = FALL_INIT;
                        }
                    } else if (PLAYER.pos.x > hit_actor->pos.x){
                        actorColX = (hit_actor->pos.x - PLAYER.pos.x) + ((-PLAYER.bounds.left + hit_actor->bounds.right)<<4)+16;
                        if (!INPUT_LEFT){
                            pl_vel_x = 0;
                        }
                        if(plat_state == DASH_STATE){
                            plat_state = FALL_INIT;
                        }
                    }
                }
            } else if (hit_actor->collision_group == plat_mp_group){
                //Platform Actors
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < hit_actor->pos.y + (hit_actor->bounds.top << 4) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;                        
                        plat_state = GROUND_INIT;
                    }
                }
            }
            //All Other Collisions
            player_register_collision_with(hit_actor);
        } else if (INPUT_PRESSED(INPUT_PLATFORM_INTERACT)) {
            if (!hit_actor) {
                hit_actor = actor_in_front_of_player(8, TRUE);
            }
            if (hit_actor && !hit_actor->collision_group && hit_actor->script.bank) {
                script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
            }
        }
    }



    //COUNTERS===============================================================
    // Counting Down Dash Frames
    if (dash_currentframe != 0){
        dash_currentframe -= 1;
    }
 
	// Counting down Jump Buffer Window
	if (jb_val != 0){
		jb_val -= 1;
	}
	// Counting down Coyote Time Window
	if (ct_val != 0 && plat_state != GROUND_STATE){
		ct_val -= 1;
	}
    //Counting down Wall Coyote Time
    if (wc_val !=0 && col == 0){
        wc_val -= 1;
    }

    // Counting down No Control frames
    if (nocontrol_h != 0){
        nocontrol_h -= 1;
    }
    
    // Counting down the drop-through floor frames
    if (nocollide != 0){
        nocollide -= 1;
    }

    // Counting down until dashing is ready again
    if (dash_ready_val != 0){
        dash_ready_val -=1;
    }

    //Counting down from the max double-tap time (left is -15, right is +15)
    if (tap_val > 0){
        tap_val -= 1;
    } else if (tap_val < 0){
        tap_val += 1;
    }

    //Hone Camera after the player has dashed
    if (camera_deadzone_x > PLATFORM_CAMERA_DEADZONE_X){
        camera_deadzone_x -= 1;
    }

    //COMPATABILITY--------------------------------------------------------------------------------------
    //For compatability with mods that check 'grounded'
    if(plat_state == GROUND_STATE || plat_state == GROUND_INIT){
        grounded = true;
    } else{
        grounded = false;
    }
}

void acceleration(BYTE dir) BANKED {
    //To remove the need for a left and right case, this function multiplies the velocity by the direction at the beginning to get its positive value, and then
    //again at the end to return its to a normal value.
    WORD tempSpd = pl_vel_x * dir;
    WORD mid_run = 0;

    if (INPUT_PLATFORM_RUN){
        switch(plat_run_type) {
            case 0:
            //Ordinay Walk (same as below)
                if(tempSpd < 0 && plat_turn_acc != 0){
                    tempSpd += plat_turn_acc;
                    tempSpd = MIN(tempSpd, plat_walk_vel);
                    run_stage = -1;
                } else{
                    run_stage = 0;
                    tempSpd += plat_walk_acc;
                    tempSpd = CLAMP(tempSpd, plat_min_vel, plat_walk_vel); 
                }
            break;
            case 1:
            //Type 1: Smooth Acceleration as the Default in GBStudio
                tempSpd += plat_run_acc;
                tempSpd = CLAMP(tempSpd, plat_min_vel, plat_run_vel);
                run_stage = 1;
            break;
            case 2:
            //Type 2: Enhanced Smooth Acceleration
                if(tempSpd < 0){
                    tempSpd += plat_turn_acc;
                    tempSpd = MIN(tempSpd, plat_walk_vel);
                    run_stage = -1;
                }
                else if (tempSpd < plat_walk_vel){
                    tempSpd += plat_walk_acc;
                    run_stage = 1;
                }
                else{
                    tempSpd += plat_run_acc;
                    tempSpd = MIN(tempSpd, plat_run_vel);
                    run_stage = 2;
                }
            break;
            case 3:
            //Type 3: Instant acceleration to full speed
                tempSpd = plat_run_vel;
                run_stage = 1;
            break;
            case 4:
            //Type 4: Tiered acceleration with 2 speeds
                //If we're below the walk speed, use walk acceleration
                if (tempSpd < 0){
                    tempSpd += plat_turn_acc;
                    tempSpd = MIN(tempSpd, plat_walk_vel);
                    run_stage = -1;
                } else if(tempSpd < plat_walk_vel){
                    tempSpd += plat_walk_acc;
                    run_stage = 1;
                } else if (tempSpd < plat_run_vel){
                //If we're above walk, but below the run speed, use run acceleration
                    tempSpd += plat_run_acc;
                    tempSpd = CLAMP(tempSpd, plat_min_vel, plat_run_vel);
                    pl_vel_x = tempSpd*dir; //We need to have this here because this part returns a different value than other sections
                    run_stage = 2;
                    deltaX = dir*plat_walk_vel;
                    return;
                } else{
                //If we're at run speed, stay there
                    run_stage = 3;
                }
            break;
            case 5:
                mid_run = (plat_run_vel - plat_walk_vel)/2;
                mid_run += plat_walk_vel;
            //Type 4: Tiered acceleration with 3 speeds
                if (tempSpd < 0){
                    tempSpd += plat_turn_acc;
                    tempSpd = MIN(tempSpd, plat_walk_vel);
                    run_stage = -1;
                }else if(tempSpd < plat_walk_vel){
                    tempSpd += plat_walk_acc;
                    run_stage = 1;
                } else if (tempSpd < mid_run){
                //If we're above walk, but below the mid-run speed, use run acceleration
                    tempSpd += plat_run_acc;
                    pl_vel_x = tempSpd*dir; //We need to have this here because this part returns a different value than other sections
                    run_stage = 2;
                    deltaX = dir*plat_walk_vel;
                    return;
                } else if (tempSpd < plat_run_vel){
                //If we're above walk, but below the run speed, use run acceleration
                    tempSpd += plat_run_acc;
                    tempSpd = CLAMP(tempSpd, plat_min_vel, plat_run_vel);
                    pl_vel_x = tempSpd*dir; //We need to have this here because this part returns a different value than other sections
                    run_stage = 3;
                    deltaX = dir*mid_run;
                    return;
                } else{
                //If we're at run speed, stay there
                    run_stage = 4;
                }
            break;
        }
    } else {
        //Ordinay Walk
        //And need to re-write the animation bit so that the buttons overpowers the velocity
        if(tempSpd < 0 && plat_turn_acc != 0){
            tempSpd += plat_turn_acc;
            tempSpd = MIN(tempSpd, plat_walk_vel);
            run_stage = -1;
        }
        else{
            run_stage = 0;
            tempSpd += plat_walk_acc;
            tempSpd = CLAMP(tempSpd, plat_min_vel, plat_walk_vel); 
        }

    }
    pl_vel_x = tempSpd*dir;
    deltaX = pl_vel_x;
}

void deceleration() BANKED {
    //Deceleration (ground and air)
    //This could be a candidate for breaking into different state machines
    if (pl_vel_x < 0) {
        if (plat_state == GROUND_STATE){
            pl_vel_x += plat_dec;
        } else { 
            pl_vel_x += plat_air_dec;
        }
        if (pl_vel_x > 0) {
            pl_vel_x = 0;
        }
    } else if (pl_vel_x > 0) {
        if (plat_state == GROUND_STATE){
            pl_vel_x -= plat_dec;
            }
        else { 
            pl_vel_x -= plat_air_dec;
            }
        if (pl_vel_x < 0) {
            pl_vel_x = 0;
        }
    }
    run_stage = 0;
}

void basic_anim() BANKED{
    //This animation is currently shared by jumping, dashing, and falling. Dashing doesn't need this complexity though.
    //Here velocity overrides direction. Whereas on the ground it is the reverse. 
    if(plat_turn_control){
        if (INPUT_LEFT){
            PLAYER.dir = DIR_LEFT;
        } else if (INPUT_RIGHT){
            PLAYER.dir = DIR_RIGHT;
        } else if (pl_vel_x < 0) {
            PLAYER.dir = DIR_LEFT;
        } else if (pl_vel_x > 0) {
            PLAYER.dir = DIR_RIGHT;
        }
    }

    if (PLAYER.dir == DIR_LEFT){
        actor_set_anim(&PLAYER, ANIM_JUMP_LEFT);
    } else {
        actor_set_anim(&PLAYER, ANIM_JUMP_RIGHT);
    }
}

void dash_check() BANKED {
    //Initialize Dash
    //Pre-Check for input and if we're already in a dash
    UBYTE tile_start, tile_end;

    //Pre-check for recharge and script interrupt
    if (dash_ready_val == 0){
        //Start Dashing!
        WORD new_x;
        plat_state = DASH_INIT;
        //If the player is pressing a direction (but not facing a direction, ie on a wall or on a changed frame)
        if (INPUT_RIGHT){
            PLAYER.dir = DIR_RIGHT;
        }
        else if(INPUT_LEFT){
            PLAYER.dir = DIR_LEFT;
        }
       
        //Set new_x be the final destination of the dash (ie. the distance covered by all of the dash frames combined)
        if (PLAYER.dir == DIR_RIGHT){
            new_x = PLAYER.pos.x + (dash_dist*plat_dash_frames);
        }
        else{
            new_x = PLAYER.pos.x + (-dash_dist*plat_dash_frames);
        }

        //Dash through walls
        if(plat_dash_through == 3 && plat_dash_momentum < 2){
            dash_end_clear = true;                              //Assume that the landing spot is clear, and disable if we collide below
            tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
            tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;     

            //Do a collision check at the final landing spot (but not all the steps in-between.)
            if (PLAYER.dir == DIR_RIGHT){
                //Don't dash off the screen to the right
                if (PLAYER.pos.x + (PLAYER.bounds.right <<4) + (dash_dist*(plat_dash_frames)) > (image_width -16) <<4){   
                    dash_end_clear = false;                                     
                } else{
                    UBYTE tile_xr = (((new_x >> 4) + PLAYER.bounds.right) >> 3) +1;  
                    UBYTE tile_xl = ((new_x >> 4) + PLAYER.bounds.left) >> 3;   
                    while (tile_xl != tile_xr){                                             //This checks all the tiles between the left bounds and the right bounds
                        while (tile_start != tile_end) {                                    //This checks all the tiles that the character occupies in height
                            if (tile_at(tile_xl, tile_start) & COLLISION_ALL) {
                                    dash_end_clear = false;
                                    goto initDash;                                          //Gotos are still good for breaking embedded loops.
                            }
                            tile_start++;
                        }
                        tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);   //Reset the height after each loop
                        tile_xl++;
                    }
                }
            } else if(PLAYER.dir == DIR_LEFT) {
                //Don't dash off the screen to the left
                if (PLAYER.pos.x <= ((dash_dist*(plat_dash_frames))+(PLAYER.bounds.left << 4))+(8<<4)){
                    dash_end_clear = false;         //To get around unsigned position, test if the player's current position is less than the total dist.
                } else{
                    UBYTE tile_xl = ((new_x >> 4) + PLAYER.bounds.left) >> 3;
                    UBYTE tile_xr = (((new_x >> 4) + PLAYER.bounds.right) >> 3) +1;  

                    while (tile_xl != tile_xr){   
                        while (tile_start != tile_end) {
                            if (tile_at(tile_xl, tile_start) & COLLISION_ALL) {
                                    dash_end_clear = false;
                                    goto initDash;
                            }
                            tile_start++;
                        }
                        tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
                        tile_xl++;
                    }
                }
            }
        }

        //INITIALIZE DASH
        initDash:
        actor_attached = FALSE;
        camera_deadzone_x = 32;
        dash_ready_val = plat_dash_ready_max + plat_dash_frames;
        if(plat_dash_momentum < 2){
            pl_vel_y = 0;
        }
        dash_currentframe = plat_dash_frames;
        tap_val = 0;
    }
}

void jump_init() BANKED {
    //Initialize Jumping
    hold_jump_val = plat_hold_jump_max; 
    actor_attached = FALSE;
    pl_vel_y = -plat_jump_min;
    jb_val = 0;
    plat_state = JUMP_INIT;

}

void ladder_check() BANKED {
    UBYTE p_half_width = (PLAYER.bounds.right - PLAYER.bounds.left) >> 1;
    if (INPUT_UP) {
        // Grab upwards ladder
        UBYTE tile_x_mid = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left + p_half_width) >> 3;
        UBYTE tile_y   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top) >> 3);
        if (tile_at(tile_x_mid, tile_y) & TILE_PROP_LADDER) {
            PLAYER.pos.x = (((tile_x_mid << 3) + 4 - (PLAYER.bounds.left + p_half_width) << 4));
            plat_state = LADDER_INIT;
            pl_vel_x = 0;
        }
    } else if (INPUT_DOWN) {
        // Grab downwards ladder
        UBYTE tile_x_mid = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left + p_half_width) >> 3;
        UBYTE tile_y   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
        if (tile_at(tile_x_mid, tile_y) & TILE_PROP_LADDER) {
            PLAYER.pos.x = (((tile_x_mid << 3) + 4 - (PLAYER.bounds.left + p_half_width) << 4));
            plat_state = LADDER_INIT;
            pl_vel_x = 0;
        }
    }
}

void wall_check() BANKED {
    //Wall-State Check
    //We need to check for ground state here because this check follows on collisions that could already have changed the state to be grounded
    if(plat_state != GROUND_STATE && pl_vel_y >= 0 && plat_wall_slide){
        if ((col == -1 && INPUT_LEFT) || (col == 1 && INPUT_RIGHT)){
            if (plat_state != WALL_STATE){
                plat_state = WALL_INIT;
            }
        } else if (plat_state == WALL_STATE){
            plat_state = FALL_INIT;
        }
    }
}

void basic_x_col() BANKED {
    
    UBYTE tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
    UBYTE tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;        
    col = 0;
    deltaX = CLAMP(deltaX, -127, 127);
    if (deltaX > 0) {
        UWORD new_x = PLAYER.pos.x + deltaX;
        UBYTE tile_x = ((new_x >> 4) + PLAYER.bounds.right) >> 3;
        while (tile_start != tile_end) {
            if (tile_at(tile_x, tile_start) & COLLISION_LEFT) {
                new_x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
                pl_vel_x = 0;
                col = 1;
                last_wall = 1;
                wc_val = plat_coyote_max;
                break;
            }
            tile_start++;
        }
        PLAYER.pos.x = MIN((image_width - 16) << 4, new_x);
    } else if (deltaX < 0) {      
        WORD new_x = PLAYER.pos.x + deltaX;
        UBYTE tile_x = ((new_x >> 4) + PLAYER.bounds.left) >> 3;
        while (tile_start != tile_end) {
            if (tile_at(tile_x, tile_start) & COLLISION_RIGHT) {
                new_x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;
                pl_vel_x = 0;
                col = -1;
                last_wall = -1;
                wc_val = plat_coyote_max;
                break;
            }
            tile_start++;
        }
        PLAYER.pos.x = MAX(0, new_x);
    }
}

void basic_y_col(UBYTE drop_press) BANKED {
    UBYTE tile_start, tile_end;
    deltaY = CLAMP(deltaY, -127, 127);
    UBYTE tempState = plat_state;
    tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
    tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
    if (deltaY > 0) {
        //Moving Downward
        WORD new_y = PLAYER.pos.y + deltaY;
        UBYTE tile_yz = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;
        if (nocollide == 0){
            while (tile_start != tile_end) {
                if (tile_at(tile_start, tile_yz) & COLLISION_TOP) {
                    //Drop-Through Floor Check 
                    UBYTE drop_attempt = FALSE;
                    if (drop_press == TRUE){
                        drop_attempt = TRUE;
                        while (tile_start != tile_end) {
                            if (tile_at(tile_start, tile_yz) & COLLISION_BOTTOM){
                                drop_attempt = FALSE;
                                break;
                            }
                        tile_start++;
                        }
                    }
                    if (drop_attempt == TRUE){
                        nocollide = 5; //Magic Number, how many frames to steal vertical control
                        pl_vel_y += plat_grav; 
                    } else {
                        //Land on Floor
                        new_y = ((((tile_yz) << 3) - PLAYER.bounds.bottom) << 4) - 1;
                        actor_attached = FALSE; //Detach when MP moves through a solid tile.
                        if(plat_state != GROUND_STATE){plat_state = GROUND_INIT;}
                        pl_vel_y = 0;
                        //Various things that reset when Grounded
                        PLAYER.pos.y = new_y;
                        return;
                    }
                }
                tile_start++;
            }
            if(plat_state == GROUND_STATE && !actor_attached){plat_state = FALL_INIT;}
        }
        PLAYER.pos.y = new_y;
    } else if (deltaY < 0) {
        //Moving Upward
        WORD new_y = PLAYER.pos.y + deltaY;
        UBYTE tile_yz = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
        while (tile_start != tile_end) {
            if (tile_at(tile_start, tile_yz) & COLLISION_BOTTOM) {
                new_y = ((((UBYTE)(tile_yz + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                pl_vel_y = 0;
                //MP Test: Attempting stuff to stop the player from continuing upward
                if(actor_attached){
                    actor_attached = FALSE;
                    new_y = last_actor->pos.y;
                }
                break;
            }
            tile_start++;
        }
        PLAYER.pos.y = new_y;
    }
    else if (actor_attached){
        plat_state = GROUND_STATE;
    }
    // Clamp Y Velocity
    pl_vel_y = CLAMP(pl_vel_y,-plat_max_fall_vel, plat_max_fall_vel);
}

