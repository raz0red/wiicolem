//---------------------------------------------------------------------------//
//   __      __.__.___________        .__                                    //
//  /  \    /  \__|__\_   ___ \  ____ |  |   ____   _____                    //
//  \   \/\/   /  |  /    \  \/ /  _ \|  | _/ __ \ /     \                   //
//   \        /|  |  \     \___(  <_> )  |_\  ___/|  Y Y  \                  //
//    \__/\  / |__|__|\______  /\____/|____/\___  >__|_|  /                  //
//         \/                \/                 \/      \/                   //
//     WiiColem by raz0red                                                   //
//     Port of the ColEm emulator by Marat Fayzullin                         //
//                                                                           //
//     [github.com/raz0red/wiicolem]                                         //
//                                                                           //
//---------------------------------------------------------------------------//
//                                                                           //
//  Copyright (C) 2019 raz0red                                               //
//                                                                           //
//  The license for ColEm as indicated by Marat Fayzullin, the author of     //
//  ColEm is detailed below:                                                 //
//                                                                           //
//  ColEm sources are available under three conditions:                      //
//                                                                           //
//  1) You are not using them for a commercial project.                      //
//  2) You provide a proper reference to Marat Fayzullin as the author of    //
//     the original source code.                                             //
//  3) You provide a link to http://fms.komkon.org/ColEm/                    //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef WII_COLECO_H
#define WII_COLECO_H

#include "wii_main.h"
#include "wii_coleco_db.h"

#define TMS9918_WIDTH 280
#define COLECO_WIDTH 272
#define COLECO_HEIGHT 200

// Default screen size
// 256x192: Coleco
// 272x200: Colem
// 280x: TMS9981 (PAR 1.143)
#define DEFAULT_SCREEN_X 732
#define DEFAULT_SCREEN_Y 480

// Wii width and height
#define WII_WIDTH 640
#define WII_HEIGHT 480
#define WII_WIDTH_DIV2 320
#define WII_HEIGHT_DIV2 240

// ColecoVision button mappings
#define WII_BUTTON_CV_SHOW_KEYPAD ( WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS )
#define GC_BUTTON_CV_SHOW_KEYPAD ( PAD_BUTTON_START )
#define WII_BUTTON_CV_RIGHT ( WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_RIGHT )
#define GC_BUTTON_CV_RIGHT ( PAD_BUTTON_RIGHT )
#define WII_BUTTON_CV_UP ( WPAD_BUTTON_RIGHT )
#define GC_BUTTON_CV_UP ( PAD_BUTTON_UP )
#define WII_CLASSIC_CV_UP ( WPAD_CLASSIC_BUTTON_UP )
#define WII_BUTTON_CV_DOWN ( WPAD_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_DOWN )
#define GC_BUTTON_CV_DOWN ( PAD_BUTTON_DOWN )
#define WII_BUTTON_CV_LEFT ( WPAD_BUTTON_UP )
#define WII_CLASSIC_CV_LEFT ( WPAD_CLASSIC_BUTTON_LEFT )
#define GC_BUTTON_CV_LEFT ( PAD_BUTTON_LEFT )

#define WII_NUNCHECK_CV_1 ( WPAD_NUNCHUK_BUTTON_C )
#define WII_BUTTON_CV_1   ( WPAD_BUTTON_2 )
#define GC_BUTTON_CV_1    ( PAD_BUTTON_A )
#define WII_CLASSIC_CV_1  ( WPAD_CLASSIC_BUTTON_A )
#define WII_NUNCHECK_CV_2 ( WPAD_NUNCHUK_BUTTON_Z )
#define WII_BUTTON_CV_2   ( WPAD_BUTTON_1 )
#define GC_BUTTON_CV_2    ( PAD_BUTTON_B )
#define WII_CLASSIC_CV_2  ( WPAD_CLASSIC_BUTTON_B )
#define WII_BUTTON_CV_3   ( WPAD_BUTTON_A )
#define GC_BUTTON_CV_3    ( PAD_BUTTON_X )
#define WII_CLASSIC_CV_3  ( WPAD_CLASSIC_BUTTON_X )
#define WII_BUTTON_CV_4   ( WPAD_BUTTON_B )
#define GC_BUTTON_CV_4    ( PAD_BUTTON_Y )
#define WII_CLASSIC_CV_4  ( WPAD_CLASSIC_BUTTON_Y )
#define GC_BUTTON_CV_5    ( PAD_TRIGGER_R )
#define WII_CLASSIC_CV_5  ( WPAD_CLASSIC_BUTTON_FULL_R )
#define GC_BUTTON_CV_6    ( PAD_TRIGGER_L )
#define WII_CLASSIC_CV_6  ( WPAD_CLASSIC_BUTTON_FULL_L )
#define WII_CLASSIC_CV_7  ( WPAD_CLASSIC_BUTTON_ZR )
#define WII_CLASSIC_CV_8  ( WPAD_CLASSIC_BUTTON_ZL )

/** The last ColecoVision cartridge hash */
extern char wii_cartridge_hash[33];
/** The current ColecoVision Mode */
extern int wii_coleco_mode;
/** The ColecoVision database entry for current game */
extern ColecoDBEntry wii_coleco_db_entry;
/** The coleco controller view mode */
extern int wii_coleco_controller_view_mode;
/** Whether to display debug info (FPS, etc.) */
extern short wii_debug;
/** Keypad pause */
extern BOOL wii_keypad_pause;
/** Keypad size */
extern u8 wii_keypad_size;
/** Overlay mode */
extern BOOL wii_use_overlay;
/** Super Game Module enabled */
extern BOOL wii_super_game_module;
/** Volume */
extern u8 wii_volume;
/** Maximum frame rate */
extern u8 wii_max_frames;
/** The screen X size */
extern int wii_screen_x;
/** The screen Y size */
extern int wii_screen_y;
/** Whether to filter the display */
extern BOOL wii_filter; 
/** Whether to use the GX/VI scaler */
extern BOOL wii_gx_vi_scaler;

/**
 * Returns the current roms directory
 *
 * @return  The current roms directory
 */
char* wii_get_roms_dir();

/**
 * Sets the current roms directory
 *
 * @param   newDir The new roms directory
 */
void wii_set_roms_dir(const char* newDir);

/**
 * Returns the saves directory
 *
 * @return  The saves directory
 */
char* wii_get_saves_dir();

/**
 * Returns the states directory
 *
 * @return  The states directory
 */
char* wii_get_states_dir();

/**
 * Returns the overlays directory
 *
 * @return  The overlays directory
 */
char* wii_get_overlays_dir();

/**
 * Checks and returns the state of the specified joystick
 *
 * @param   joyIndex The index of the joystick to check
 * @return  The current state of the specified joystick
 */
u32 wii_coleco_poll_joystick(int joyIndex);

/**
 * Updates whether the Wii is in widescreen mode
 */
void wii_update_widescreen();

/**
 * Sets the video mode for the Wii
 *
 * @param   allowVi Whether to allow the GX+VI mode
 */
void wii_set_video_mode(BOOL allowVi);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the current size to use for the screen
 *
 * @param   inX Input x value
 * @param   inY Input y value
 * @param   x (out) Output x value
 * @param   y (out) Output y value
 */
void wii_get_screen_size(int inX, int inY, int *x, int *y);

#ifdef __cplusplus
}
#endif

#endif
