/*
WiiColem : Port of the Colem ColecoVision Emulator for the Wii

Copyright (C) 2010
raz0red (www.twitchasylum.com)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.
*/

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
