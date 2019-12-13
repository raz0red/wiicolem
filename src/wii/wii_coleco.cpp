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

#include "Coleco.h"

#include "wii_app_common.h"
#include "wii_app.h"
#include "wii_config.h"
#include "wii_input.h"
#include "wii_sdl.h"
#include "wii_coleco.h"
#include "wii_coleco_keypad.h"
#include "wii_coleco_menu.h"
#include "wii_gx.h"
#include "wii_main.h"

#include "FreeTypeGX.h"
#include "font_ttf.h"

#ifdef WII_NETTRACE
#include <network.h>
#include "net_print.h"  
#endif

/** SDL Video external references */
extern "C" {
void WII_SetWidescreen(int wide);
void WII_VideoStop();
void WII_VideoStart();
void WII_ChangeSquare(int xscale, int yscale, int xshift, int yshift);
void WII_SetFilter(BOOL filter);
void WII_SetDefaultVideoMode();
void WII_SetStandardVideoMode(int xscale, int yscale, int width);
void WII_SetDoubleStrikeVideoMode(int xscale, int yscale, int width);
void WII_SetInterlaceVideoMode(int xscale, int yscale, int width);
}

/** The last ColecoVision cartridge hash */
char wii_cartridge_hash[33];
/** The ColecoVision database entry for current game */
ColecoDBEntry wii_coleco_db_entry;
/** The current ColecoVision Mode */
int wii_coleco_mode;
/** The coleco controller view mode */
int wii_coleco_controller_view_mode = 0;
/** Whether to display debug info (FPS, etc.) */
short wii_debug = 0;
/** Keypad pause */
BOOL wii_keypad_pause = TRUE;
/** Keypad size */
u8 wii_keypad_size = KEYPAD_SIZE_MED;
/** Overlay mode */
BOOL wii_use_overlay = TRUE;
/** Super Game Module enabled */
BOOL wii_super_game_module = TRUE;
/** Volume */
u8 wii_volume = 7;
/** Maximum frame rate */
u8 wii_max_frames = 60;
/** The screen X size */
int wii_screen_x = DEFAULT_SCREEN_X;
/** The screen Y size */
int wii_screen_y = DEFAULT_SCREEN_Y;
/** Whether to filter the display */
BOOL wii_filter = FALSE;
/** Whether to use the GX/VI scaler */
BOOL wii_gx_vi_scaler = TRUE;
/** The current roms directory */
static char roms_dir[WII_MAX_PATH] = "";
/** The states directory */
static char states_dir[WII_MAX_PATH] = "";
/** The saves directory */
static char saves_dir[WII_MAX_PATH] = "";
/** The overlays directory */
static char overlays_dir[WII_MAX_PATH] = "";

/**
 * Returns the base directory for the application
 * 
 * @return  The base directory for the application
 */
const char* wii_get_app_base_dir() {
    return WII_BASE_APP_DIR;
}

/**
 * Returns the location of the config file
 * 
 * @return  The location of the config file
 */
const char* wii_get_config_file_path() {
    return WII_CONFIG_FILE;
}

/**
 * Returns the location of the data directory
 * 
 * @return  The location of the data directory
 */
const char* wii_get_data_path() {
    return WII_FILES_DIR;
}

/**
 * Initializes the application
 */
void wii_handle_init() {
    wii_read_config();

    // Startup the SDL
    if (!wii_sdl_init()) {
        fprintf(stderr, "FAILED : Unable to init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // FreeTypeGX
    InitFreeType((uint8_t*)font_ttf, (FT_Long)font_ttf_size);

    // Update whether the Wii is in widescreen mode
    wii_update_widescreen();

    // Initialize the colecovision menu
    wii_coleco_menu_init();
}

/**
 * Updates whether the Wii is in widescreen mode
 */
void wii_update_widescreen() {
    int ws =
        (wii_full_widescreen == WS_AUTO ? is_widescreen : wii_full_widescreen);

    // Set video to appropriate widescreen adjust
    WII_SetWidescreen(ws);
}

/**
 * Returns the current size to use for the screen
 *
 * @param   inX Input x value
 * @param   inY Input y value
 * @param   x (out) Output x value
 * @param   y (out) Output y value
 */
void wii_get_screen_size(int inX, int inY, int* x, int* y) {
    int xs = inX;
    int ys = inY;

    // 4:3 correct is applicable and enabled
    if (wii_16_9_correction == WS_AUTO ? is_widescreen : wii_16_9_correction) {
        xs = (xs * 3) / 4;  // Widescreen correct
    }

    // Round up
    xs = ((xs + 1) & ~1);
    ys = ((ys + 1) & ~1);

    // Set output values
    *x = xs;
    *y = ys;
}

/**
 * Sets the video mode for the Wii
 *
 * @param   allowVi Whether to allow VI-based modes (GX+VI and Double-strike)
 */
void wii_set_video_mode(BOOL allowVi) {
    // Get the screen size
    int x, y;
    wii_get_screen_size(wii_screen_x, wii_screen_y, &x, &y);

    if (allowVi && wii_double_strike_mode) {
        WII_SetDoubleStrikeVideoMode(x, y >> 1, TMS9918_WIDTH);
    } else if (allowVi && wii_gx_vi_scaler) {
        // VI+GX
        WII_SetStandardVideoMode(x, y, TMS9918_WIDTH);
    } else {
        // Scale the screen (GX)
        WII_SetDefaultVideoMode();
        WII_SetFilter(wii_filter);
        WII_ChangeSquare(x, y, 0, 0);
    }
}

/**
 * Frees resources prior to the application exiting
 */
void wii_handle_free_resources() {
    wii_sdl_free_resources();
    wii_keypad_free_resources();
    SDL_Quit();
}

/**
 * Runs the application (main loop)
 */
void wii_handle_run() {
    /* ColEm settings */
    Verbose = 0;
    UPeriod = 100;

    // Initialize and run ColEm
    if (InitMachine()) {
        StartColeco(NULL);

        // Clear the screen prior to exiting
        wii_sdl_black_screen();

        if (wii_gx_vi_scaler || wii_double_strike_mode) {
            WII_VideoStop();
            wii_gx_push_callback( NULL, FALSE, NULL );    
            WII_SetDefaultVideoMode();
            VIDEO_WaitVSync();
            WII_VideoStart(); 
            VIDEO_WaitVSync();
        }

        TrashColeco();
        TrashMachine();
    }

    // Write the config settings, free resources, and exit
    wii_write_config();
}

/**
 * Returns the current roms directory
 *
 * @return  The current roms directory
 */
char* wii_get_roms_dir() {
    return roms_dir;
}

/**
 * Sets the current roms directory
 *
 * @param   newDir The new roms directory
 */
void wii_set_roms_dir(const char* newDir) {
    Util_strlcpy(roms_dir, newDir, sizeof(roms_dir));
#ifdef WII_NETTRACE
    net_print_string(NULL, 0, "RomsDir: \"%s\"\n", roms_dir);
#endif
}

/**
 * Returns the saves directory
 *
 * @return  The saves directory
 */
char* wii_get_saves_dir() {
    if (saves_dir[0] == '\0') {
        snprintf(saves_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 WII_SAVES_DIR);
    }
    return saves_dir;
}

/**
 * Returns the states directory
 *
 * @return  The states directory
 */
char* wii_get_states_dir() {
    if (states_dir[0] == '\0') {
        snprintf(states_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 WII_STATES_DIR);
    }
    return states_dir;
}

/**
 * Returns the overlays directory
 *
 * @return  The overlays directory
 */
char* wii_get_overlays_dir() {
    if (overlays_dir[0] == '\0') {
        snprintf(overlays_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 WII_OVERLAYS_DIR);
    }
    return overlays_dir;
}

/**
 * Returns the button value for the specified button index
 *
 * @param   current The current state of the joystick
 * @param   buttonIndex The button to return the value for
 * @return  The button value for the specified index
 */
static u32 get_button_value(u32 current, u8 buttonIndex) {
    u32 val = wii_coleco_db_entry.button[buttonIndex];

    // If the value isn't related to the keypad, return it.
    // If it is related to the keypad, ensure no other keypad
    // button is currently pressed.
    return (!(val & JST_KEYPAD) ? val : ((current & JST_KEYPAD) ? 0 : val));
}

/**
 * Checks and returns the state of the specified joystick
 *
 * @param   joyIndex The index of the joystick to check
 * @return  The current state of the specified joystick
 */
u32 wii_coleco_poll_joystick(int joyIndex) {
    u32 I = 0;

    // Check the state of the controllers
    u32 held = WPAD_ButtonsHeld(joyIndex);
    u32 gcHeld = PAD_ButtonsHeld(joyIndex);

    expansion_t exp;
    WPAD_Expansion(joyIndex, &exp);
    u8 isClassic = (exp.type == WPAD_EXP_CLASSIC);

    // Whether we allow input from the left analog joystick
    s8 allowAnalogLjs =
        (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_STANDARD ||
         wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION);

    // Whether we allow input from the right analog joystick
    s8 allowAnalogRjs =
        (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_STANDARD) ||
        ((wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION) &&
         (wii_coleco_db_entry.flags & DISABLE_SPINNER));

    // Whether the Wiimote is horizontal
    u8 wmHorizontal = wii_coleco_db_entry.wiiMoteHorizontal;

    // Whether Keypad as D-Pad is enabled
    u8 kPadAsDpad = wii_coleco_db_entry.flags & KPAD_AS_DPAD;

    float expX = 0, expY = 0, expRjsX = 0, expRjsY = 0;
    s8 gcX = 0, gcY = 0, gcRjsX = 0, gcRjsY = 0;

    if (allowAnalogLjs) {
        if (isClassic) {
            expX = wii_exp_analog_val(&exp, TRUE, FALSE);
            expY = wii_exp_analog_val(&exp, FALSE, FALSE);
        }
        gcX = PAD_StickX(joyIndex);
        gcY = PAD_StickY(joyIndex);
    }

    if (allowAnalogRjs) {
        expRjsX = wii_exp_analog_val(&exp, TRUE, TRUE);
        expRjsY = wii_exp_analog_val(&exp, FALSE, TRUE);
        gcRjsX = PAD_SubStickX(joyIndex);
        gcRjsY = PAD_SubStickY(joyIndex);
    }

    // Right
    if (wii_digital_right(wmHorizontal, 1, held) ||
        (gcHeld & GC_BUTTON_CV_RIGHT) || (wii_analog_right(expX, gcX)) ||
        (!kPadAsDpad && wii_analog_right(expRjsX, gcRjsX))) {
        I |= JST_RIGHT;
    }
    // Left
    if (wii_digital_left(wmHorizontal, 0, held) ||
        (isClassic ? (held & WII_CLASSIC_CV_LEFT) : 0) ||
        (gcHeld & GC_BUTTON_CV_LEFT) || (wii_analog_left(expX, gcX)) ||
        (!kPadAsDpad && wii_analog_left(expRjsX, gcRjsX))) {
        I |= JST_LEFT;
    }
    // Down
    if (wii_digital_down(wmHorizontal, 1, held) ||
        (gcHeld & GC_BUTTON_CV_DOWN) || (wii_analog_down(expY, gcY)) ||
        (!kPadAsDpad && wii_analog_down(expRjsY, gcRjsY))) {
        I |= JST_DOWN;
    }
    // Up
    if (wii_digital_up(wmHorizontal, 0, held) ||
        (isClassic ? (held & WII_CLASSIC_CV_UP) : 0) ||
        (gcHeld & GC_BUTTON_CV_UP) || (wii_analog_up(expY, gcY)) ||
        (!kPadAsDpad && wii_analog_up(expRjsY, gcRjsY))) {
        I |= JST_UP;
    }
    // Button 1
    if (held & (WII_BUTTON_CV_1 |
                (isClassic ? WII_CLASSIC_CV_1 : WII_NUNCHECK_CV_1)) ||
        gcHeld & GC_BUTTON_CV_1) {
        I |= get_button_value(I, 0);
    }
    // Button 2
    if (held & (WII_BUTTON_CV_2 |
                (isClassic ? WII_CLASSIC_CV_2 : WII_NUNCHECK_CV_2)) ||
        gcHeld & GC_BUTTON_CV_2) {
        I |= get_button_value(I, 1);
    }
    // Button 3
    if (held & (WII_BUTTON_CV_3 | WII_CLASSIC_CV_3) ||
        gcHeld & GC_BUTTON_CV_3) {
        I |= get_button_value(I, 2);
    }
    // Button 4
    if (held & (WII_BUTTON_CV_4 | WII_CLASSIC_CV_4) ||
        gcHeld & GC_BUTTON_CV_4) {
        I |= get_button_value(I, 3);
    }
    // Button 5
    if (held & WII_CLASSIC_CV_5 || gcHeld & GC_BUTTON_CV_5) {
        I |= get_button_value(I, 4);
    }
    // Button 6
    if (held & WII_CLASSIC_CV_6 || gcHeld & GC_BUTTON_CV_6) {
        I |= get_button_value(I, 5);
    }
    // Button 7
    if (held & WII_CLASSIC_CV_7) {
        I |= get_button_value(I, 6);
    }
    // Button 8
    if (held & WII_CLASSIC_CV_8) {
        I |= get_button_value(I, 7);
    }

    // Only if in Keypad as D-PAD via Analog mode and
    // a keypad button has not been pressed
    if (kPadAsDpad && !(I & JST_KEYPAD)) {
        if (wii_analog_left(expRjsX, gcRjsX)) {
            I |= wii_analog_up(expRjsY, gcRjsY)
                     ? JST_1
                     : wii_analog_down(expRjsY, gcRjsY) ? JST_7 : JST_4;
        } else if (wii_analog_right(expRjsX, gcRjsX)) {
            I |= wii_analog_up(expRjsY, gcRjsY)
                     ? JST_3
                     : wii_analog_down(expRjsY, gcRjsY) ? JST_9 : JST_6;
        } else if (wii_analog_up(expRjsY, gcRjsY)) {
            I |= JST_2;
        } else if (wii_analog_down(expRjsY, gcRjsY)) {
            I |= JST_8;
        }
    }

    return I;
}
