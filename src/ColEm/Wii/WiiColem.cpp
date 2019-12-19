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
///////////////////////////////////////////////////////////////////////////////

/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                          ColEm.c                        **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2007                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wiiuse/wpad.h>

#include "Coleco.h"
#include "Sound.h"

#include "fileop.h"
#include "vi_encoder.h"
#include "wii_sdl.h"
#include "wii_app.h"
#include "wii_hw_buttons.h"
#include "wii_gx.h"
#include "wii_coleco.h"
#include "wii_input.h"
#include "wii_coleco_keypad.h"
#include "wii_coleco_emulation.h"
#include "wii_main.h"

// Sound information
static int UseSound = 48000;   /* Audio sampling frequency (Hz)  */
static int SndSwitch;          /* Mask of enabled sound channels */
static int SndVolume;          /* Master volume for audio        */

// Cycle timing information
static u64 TicksPerUpdate;
static u64 TicksPerSecond;
static u64 NextTick;
static u64 CurrentTick;
static u64 TimerCount;
static u64 StartTick;

/** Whether to reset the timing information */
static BOOL ResetTiming = TRUE;

/** The current FPS */
static float FpsCounter = 0.0;

/** Is this the first time the loop is occurring */
static BOOL InitialLoop = TRUE;

/** Forward reference to render the emulator screen */
static void render_screen();

/** View external reference (SDL) */
extern Mtx gx_view;

/** For logging debug info */
char debug_str[255] = "";

extern "C" {
/** SDL Sound external reference */
int PauseAudio(int Switch);
/** SDL Video external references */
void WII_VideoStop();
void WII_VideoStart();
void WII_SetDefaultVideoMode();
}

// Forward references
static void wii_render_callback();
static void wii_prerender_callback();

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void) {
    TrashSound();
}

/** ResetTiming() ********************************************/
/** Resets cycle timing information                         **/
/*************************************************************/
void ResetCycleTiming(void) {
    TicksPerSecond = 1000 * 100;
    // int UpdateFreq = wii_max_frames/*(Mode & CV_PAL) == CV_NTSC ? 60 : 50*/;
    int UpdateFreq = (wii_coleco_db_entry.maxFrames == MAX_FRAMES_DEFAULT
                          ? wii_max_frames
                          : wii_coleco_db_entry.maxFrames);

    TicksPerUpdate = TicksPerSecond / UpdateFreq;
    ResetTiming = TRUE;
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void) {
    /* Initialize video */
    ScrWidth = COLECO_WIDTH;
    ScrHeight = COLECO_HEIGHT;
    ScrBuffer = blit_surface->pixels;

    // Reset timing information
    ResetCycleTiming();

    InitSound(UseSound, 150);
    SndSwitch = (1 << (SN76489_CHANNELS + AY8910_CHANNELS)) - 1;
    SndVolume = 255 / SN76489_CHANNELS;
    SetChannels(SndVolume, SndSwitch);

    return (1);
}

int MouseX = 0;
int MouseY = 0;
float Tilt = 0.0;

/** Mouse() **************************************************/
/** This function is called to poll mouse. It should return **/
/** the X coordinate (-512..+512) in the lower 16 bits, the **/
/** Y coordinate (-512..+512) in the next 14 bits, and the  **/
/** mouse buttons in the upper 2 bits. All values should be **/
/** counted from the screen center!                         **/
/*************************************************************/
unsigned int Mouse(void) {
    int y = 0;
    int x = 0;

    MouseX = MouseY = 0;

    if (wii_coleco_db_entry.controlsMode != CONTROLS_MODE_STANDARD) {
        if (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION) {
            if (!(wii_coleco_db_entry.flags & DISABLE_SPINNER)) {
                WPADData* wd0 = WPAD_Data(0);
                WPADData* wd1 = WPAD_Data(1);

                if (!wii_keypad_is_visible(0)) {
                    x = ((wd0->exp.type == WPAD_EXP_CLASSIC ||
                          wd0->exp.type == WPAD_EXP_NUNCHUK)
                             ? ((int)wii_exp_analog_val_range(&(wd0->exp), 1, 1,
                                                              512.0f))
                             : (PAD_SubStickX(0) << 2));
                    x *= -1;
                }

                if (!wii_keypad_is_visible(1)) {
                    y = ((wd1->exp.type == WPAD_EXP_CLASSIC ||
                          wd1->exp.type == WPAD_EXP_NUNCHUK)
                             ? ((int)wii_exp_analog_val_range(&(wd1->exp), 1, 1,
                                                              128.0f))
                             : (PAD_SubStickX(1)));
                }
            }
        } else if (!wii_keypad_is_visible(0)) {
            WPADData* wd = WPAD_Data(0);
            if (wii_coleco_db_entry.controlsMode ==
                CONTROLS_MODE_DRIVING_TILT) {
                orient_t orient;
                WPAD_Orientation(0, &orient);
                Tilt = orient.pitch;

                // Calculate sensitivity (10-50)
                int max = (10 + (wii_coleco_db_entry.sensitivity - 1) * 5);
                float ratio = ((float)512 / (float)max);

                if (Tilt > max)
                    Tilt = max;
                else if (Tilt < -max)
                    Tilt = -max;

                x = -(Tilt * ratio);
            } else {
                // Calculate sensitivity (48-128)
                int sensitivity =
                    (128 - (wii_coleco_db_entry.sensitivity - 1) * 10);
                if (wd->exp.type == WPAD_EXP_CLASSIC ||
                    wd->exp.type == WPAD_EXP_NUNCHUK) {
                    if (wii_coleco_db_entry.controlsMode ==
                        CONTROLS_MODE_ROLLER) {
                        y = ((int)wii_exp_analog_val_range(&(wd->exp), 0, 0,
                                                           sensitivity) *
                             -1);
                    }
                    x = ((int)wii_exp_analog_val_range(&(wd->exp), 1, 0,
                                                       sensitivity << 2));
                } else {
                    if (wii_coleco_db_entry.controlsMode ==
                        CONTROLS_MODE_ROLLER) {
                        float yf = (float)PAD_StickY(0) / 100.0;
                        if (yf > 1.0)
                            yf = 1.0;
                        y = (yf * (-sensitivity));
                    }
                    float xf = (float)PAD_StickX(0) / 100.0;
                    if (xf > 1.0)
                        xf = 1.0;
                    x = (((int)(xf * sensitivity)) << 2);
                }
            }
        }

        MouseX = x;
        MouseY = y;

        x &= 0xFFFF;
        y &= 0x3FFF;

        return ((y << 16) | x);
    }

    return 0;
}

/**
 * Adds emulator render callback. This should be called after the menu has
 * been displayed.
 */
static void AddRenderCallbackPostMenu() {
    if (!ExitNow) { 
        WII_VideoStop();     
        wii_gx_pop_callback(); // Added by menu to blank screen                                                   
        wii_set_video_mode(TRUE);              
        wii_gx_push_callback(&wii_render_callback, TRUE, &wii_prerender_callback);                        
        VIDEO_SetTrapFilter(wii_trap_filter);
        VIDEO_WaitVSync();
        WII_VideoStart();                                                        
    }
}

/**
 * Removes emulator render callback. This should be called prior to the menu
 * being displayed.
 */
static void RemoveRenderCallbackPreMenu() {
    WII_VideoStop();                                                        
    wii_gx_pop_callback(); // Emulator render callback
    wii_gx_push_callback( NULL, FALSE, NULL ); // Blank screen   
    WII_SetDefaultVideoMode();
    VIDEO_WaitVSync();
    WII_VideoStart(); 
    VIDEO_WaitVSync();
    wii_gx_pop_callback(); // Blank screen callback
}

/** Joystick() ***********************************************/
/** This function is called to poll joysticks. It should    **/
/** return 2x16bit values in a following way:               **/
/**                                                         **/
/**      x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0      **/
/**                                                         **/
/** Least significant bit on the right. Active value is 1.  **/
/*************************************************************/

// The last keypad button pressed when we were in the "pause emulation
// when keypad is displayed mode".
static u32 lastkeypad = 0;
static BOOL keypad_changed = FALSE;

unsigned int Joystick(void) {
    /* Render audio here */
    RenderAndPlayAudio(GetFreeAudio());

    if (InitialLoop) {
        InitialLoop = FALSE;

        if (wii_initial_rom[0] != '\0') {
            // Launch the initial rom (Wiiflow)
            AddRenderCallbackPostMenu(); // Sets correct video mode
            wii_start_emulation(wii_initial_rom, NULL, FALSE, FALSE);
        } else {
            // Show the menu
            wii_menu_show();
            // Adds emulator render callback after menu
            AddRenderCallbackPostMenu();
        }
    }

    u32 keypad = 0;
    BOOL loop = FALSE;
    BOOL padvisible = FALSE;
    do {
        loop = FALSE;

        WPAD_ScanPads();
        PAD_ScanPads();

        //
        // A key was pressed when we were in the "pause emulation when keypad
        // is displayed mode". We keep returning the key that was pressed
        // until the button is no longer held.
        //
        if (lastkeypad) {
            if (keypad_changed || wii_is_any_button_held(2)) {
                keypad_changed = FALSE;
                return lastkeypad;
            } else {
                lastkeypad = 0;
                keypad_changed = FALSE;
            }
        }

        u32 down = WPAD_ButtonsDown(0);
        u32 gcDown = PAD_ButtonsDown(0);

        //
        // Check to see if the menu should be displayed
        //
        if ((down & WII_BUTTON_HOME) || (gcDown & GC_BUTTON_HOME) ||
            wii_hw_button) {
            // Removes the emulator render callback prior to showing the menu
            RemoveRenderCallbackPreMenu();
            // Show the menu
            wii_menu_show();
            // Adds emulator render callback after menu
            AddRenderCallbackPostMenu();
        }

        //
        // Check keypad input
        //
        keypad = wii_keypad_poll_joysticks();

        if (wii_keypad_is_pause_enabled(&wii_coleco_db_entry) ||
            (wii_gx_vi_scaler || wii_double_strike_mode)) {
            if (wii_keypad_is_visible(0) || wii_keypad_is_visible(1)) {
                if (keypad) {
                    // Hide the keypads
                    if (wii_keypad_is_pressed(0, keypad))
                        wii_keypad_hide(0);
                    if (wii_keypad_is_pressed(1, keypad))
                        wii_keypad_hide(1);

                    // Repeat the last keypad press until the button is released
                    lastkeypad = keypad;
                    keypad_changed = TRUE;
                } else {
                    // A keypad is displayed, render the screen and pads
                    // and loop again, waiting for the keypads to close
                    // or a button to be pressed.
                    if (!padvisible) {
                        padvisible = TRUE;

                        // Dim the screen
                        wii_keypad_set_dim_screen(TRUE);

                        // Force GX without VI
                        if (wii_gx_vi_scaler || wii_double_strike_mode) {                                                        
                            WII_VideoStop();                            
                            wii_set_video_mode(FALSE);
                            VIDEO_WaitVSync();
                            WII_VideoStart();                                                        
                        }
                    }

                    // Make sure audio is paused
                    PauseAudio(1);
                    // Render the screen
                    render_screen();

                    if (!ExitNow) {
                        loop = TRUE;
                    }
                }
            }

            if (!loop && padvisible) {
                // Restart selected video mode
                if (wii_gx_vi_scaler || wii_double_strike_mode) {
                    WII_VideoStop();                            
                    wii_set_video_mode(TRUE);
                    VIDEO_WaitVSync();
                    WII_VideoStart();                                                        
                }

                // Un-dim the screen
                wii_keypad_set_dim_screen(FALSE);

                // The pad was visible so we looped one or more times.
                // Reset the timing info.
                PauseAudio(0);
                ResetCycleTiming();
            }
        }
    } while (loop);

    if (keypad) {
        return keypad;
    } else {
        if (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_ROLLER ||
            wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING ||
            wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING_TILT) {
            if (wii_keypad_is_visible(0)) {
                return 0;
            } else {
                // 8   4    2 1 8 4 2 1 8   4    2 1 8  4  2  1
                // x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0
                int state = wii_coleco_poll_joystick(0);
                return (((state & 0x0F0F) << 16) | (state & 0x4040) |
                        (state & 0x40400000));
            }
        } else {
            return (wii_keypad_is_visible(0)
                        ? 0
                        : (wii_coleco_poll_joystick(0) & 0xFFFF)) |
                   (wii_keypad_is_visible(1)
                        ? 0
                        : ((wii_coleco_poll_joystick(1) & 0xFFFF) << 16));
        }
    }
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
int SetColor(byte N, byte R, byte G, byte B) {
    return wii_sdl_rgb(R, G, B);
}

/**
 * GX pre-render callback
 * */
static void wii_prerender_callback() {
    if (wii_usb_keepalive) {
        UsbKeepAlive();  // Attempt to keep the USB drive from sleeping...
    }
}

/**
 * GX render callback
 */
static void wii_render_callback() {
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);

    Mtx m;   // model matrix.
    Mtx mv;  // modelview matrix.

    guMtxIdentity(m);
    guMtxTransApply(m, m, 0, 0, -100);
    guMtxConcat(gx_view, m, mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);

    //
    // Keypad
    //

    wii_keypad_render();

    //
    // Debug
    //

    static int dbg_count = 0;

    if (wii_debug && !wii_double_strike_mode && !wii_gx_vi_scaler) {
        static char text[256] = "";
        static char text2[256] = "";
        dbg_count++;

        int pixelSize = 14;
        int h = pixelSize;
        int padding = 2;

        if (dbg_count % 60 == 0) {
#ifdef ENABLE_VSYNC            
            sprintf(text, "FPS: %0.2f, VSync: %s, CycleAdj: %d %s", FpsCounter,
                    (wii_vsync == VSYNC_ENABLED ? "On" : "Off"),
                    wii_coleco_db_entry.cycleAdjust, debug_str);
#else
            sprintf(text, "FPS: %0.2f, CycleAdj: %d %s", FpsCounter,                    
                    wii_coleco_db_entry.cycleAdjust, debug_str);
#endif                    
        }

        GXColor color = (GXColor){0x0, 0x0, 0x0, 0x80};
        int w = wii_gx_gettextwidth(pixelSize, text);
        int x = -310;
        int y = 196;

        wii_gx_drawrectangle(x + -padding, y + h + padding, w + (padding << 1),
                             h + (padding << 1), color, TRUE);

        wii_gx_drawtext(x, y, pixelSize, text, ftgxWhite, FTGX_ALIGN_BOTTOM);

        if (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING_TILT) {
            sprintf(text2, "Tilt: %0.2f", Tilt);

            int w = wii_gx_gettextwidth(pixelSize, text2);
            int x = -310;
            int y = -210;

            wii_gx_drawrectangle(x + -padding, y + h + padding,
                                 w + (padding << 1), h + (padding << 1), color,
                                 TRUE);

            wii_gx_drawtext(x, y, pixelSize, text2, ftgxWhite,
                            FTGX_ALIGN_BOTTOM);
        }
    }
}

/**
 * Renders the screen
 */
static void render_screen() {
    // Normal rendering
    wii_sdl_put_image_normal(1);

    // Send to display
    wii_sdl_flip();

#ifdef ENABLE_VSYNC
    // Wait for VSync signal
    if (wii_vsync == VSYNC_ENABLED)
        VIDEO_WaitVSync();
#endif        
}

/**
 * Returns the ticks since SDL initialization
 *
 * @return  The ticks since SDL initialization
 */
static u64 getTicks() {
    u64 ticks = SDL_GetTicks();
    return ticks * 100;
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void* Buffer, int Width, int Height) {
    // Render the screen
    render_screen();

    // Wait if needed
    if (ResetTiming) {
        NextTick = getTicks() + TicksPerUpdate;

        if (wii_debug) {
            TimerCount = 0;
            StartTick = getTicks();
        }

        ResetTiming = FALSE;
    } else {
        do {
            CurrentTick = getTicks();
        } while (CurrentTick < NextTick);
        NextTick += TicksPerUpdate;

        if (wii_debug) {
            FpsCounter =
                (((float)TimerCount++ / (CurrentTick - StartTick)) * 100000.0);
        }
    }
}
