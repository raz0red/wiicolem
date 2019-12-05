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

#ifndef WII_COLECO_KEYPAD_H
#define WII_COLECO_KEYPAD_H

#include <gctypes.h>

/*
 * Renders the keypads
 */
extern void wii_keypad_render();

/*
 * Polls the joysticks and returns state information regarding keypad 
 * selection.
 *
 * return   State indicating keypad selection
 */
extern u32 wii_keypad_poll_joysticks();

/*
 * Check whether the specified keypad is visble (0-based)
 *
 * keypadIndex  The keypad to check
 * return       Whether the specified keypad is visible
 */
extern BOOL wii_keypad_is_visible( int keypadIndex );

/*
 * Hides the specified keypad
 *
 * keypadIndex    The keypad to hide
 */
extern void wii_keypad_hide( int keypadIndex );

/*
 * Hides both of the keypads
 */
extern void wii_keypad_hide_both();

/*
 * Returns whether the value specified contains a key press for the given
 * keypad.
 *
 * return   Whether the value specified contains a key press for the
 *          given keypad.
 */ 
extern BOOL wii_keypad_is_pressed( int keypadIndex, u32 value );

/*
 * Whether to pause the keypad if it is displayed
 *
 * entry    The database entry
 * return   Whether to pause the keypad if it is displayed
 */
extern BOOL wii_keypad_is_pause_enabled( ColecoDBEntry *entry );

/*
 * Resets the keypads 
 */
extern void wii_keypad_reset();

/*
 * Frees the keypad resources
 */
extern void wii_keypad_free_resources();

/*
 * Whether to dim the screen when the keypads are displayed
 *
 * dim  Whether to dim the screen when the keypads are displayed
 */
extern void wii_keypad_set_dim_screen( BOOL dim );

#endif
