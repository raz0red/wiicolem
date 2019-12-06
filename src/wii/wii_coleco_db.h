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

#ifndef WII_COLECO_DB
#define WII_COLECO_DB

#include <gctypes.h>

#include "wii_util.h"

// Controls modes
#define CONTROLS_MODE_STANDARD      0
#define CONTROLS_MODE_SUPERACTION   1
#define CONTROLS_MODE_DRIVING       2
#define CONTROLS_MODE_DRIVING_TILT  3
#define CONTROLS_MODE_ROLLER        4

// Cartridge flags
#define OPCODE_MEMORY           0x00000001
#define CART_SRAM               0x00000002
#define IGNORE_R4_MASK          0x00000004 // No longer used
#define DISABLE_SPINNER         0x00000008
#define KPAD_AS_DPAD            0x00000010

// Keypad pause
#define KEYPAD_PAUSE_DEFAULT    0
#define KEYPAD_PAUSE_ENABLED    1
#define KEYPAD_PAUSE_DISABLED   2

// Keypad size
#define KEYPAD_SIZE_DEFAULT     0
#define KEYPAD_SIZE_SMALL       1
#define KEYPAD_SIZE_MED         2
#define KEYPAD_SIZE_LARGE       3

// EEPROM
#define EEPROM_NONE     0
#define EEPROM_24C08    1
#define EEPROM_24C256   2

// Max frames default
#define MAX_FRAMES_DEFAULT      0

// Overlay
#define OVERLAY_DEFAULT    0
#define OVERLAY_ENABLED    1
#define OVERLAY_DISABLED   2

// The maximum number of buttons that can be mapped
#define MAPPED_BUTTON_COUNT 8

// The count of controller names
#define CONTROLLER_NAME_COUNT 5

/** The count of Coleco button names */
#define COLECO_BUTTON_NAME_COUNT 19

/*
 * Coleco overlay 
 */
typedef struct ColecoOverlay
{
  char keypOverlay[WII_MAX_PATH];   // The keypad overlay
  RGBA keypSelKeyRgba;  // The keypad selected key color
  u16 keypSelKeyW;      // The keypad selected key width
  u16 keypSelKeyH;      // The keypad selected key height
  u16 keypOffX;         // The keypad x offset
  u16 keypOffY;         // The keypad y offset
  u16 keypGapX;         // The keypad x gap
  u16 keypGapY;         // The keypad y gap
} ColecoOverlay;

/*
 * Coleco database entry 
 */
typedef struct ColecoDBEntry
{
  char name[255];       // The name of the game
  u8 controlsMode;      // The controller mode
  u8 wiiMoteHorizontal; // Whether the WiiMote is horizontal  
  u32 flags;            // Special flags
  u8 loaded;            // Whether the settings were loaded 
  u8 sensitivity;       // How sensitive the controller is
  u8 keypadPause;       // Whether keypad display should pause emulator
  u8 keypadSize;        // The size of the keypad
  u8 overlayMode;       // Whether the overlay is enabled/disabled/default
  s32 cycleAdjust;      // Adjust the cycles per scanline (hack)
  s32 maxFrames;        // Maximum frames
  u8 eeprom;            // EEPROM
  u32 button[MAPPED_BUTTON_COUNT];  // Button mappings  
  char buttonDesc[COLECO_BUTTON_NAME_COUNT][255]; // Button description
  ColecoOverlay overlay;
} ColecoDBEntry;

/*
 * Structure for mapping button values to button names 
 */
typedef struct ColecoButtonName
{ 
  u32 button;   // The button value
  const char *name;   // The button name
} ColecoButtonName;

/*
 * Descriptions of the different Wii mappable buttons. 
 *
 * It is important to note that a particular "button" index is mapped to a 
 * ColecoVision button. For example, if "Button 1" were mapped to the 
 * Coleco's right fire button. This means that the WiiMote's 1 
 * button, the NunChuck's Z button, the GameCube's B button and the 
 * Classic Controller's B button will be mapped to the right fire button.
 *
 * The assignments of Wii controller buttons to their assigned "button" 
 * index (1-8) cannot be changed.
 */
extern const char* WiiButtonDescriptions[MAPPED_BUTTON_COUNT][CONTROLLER_NAME_COUNT];

/*
 * The names of the Wii controllers 
 */
extern const char* WiiControllerNames[CONTROLLER_NAME_COUNT];

/*
 * The Coleco button values and their associated names
 */
extern ColecoButtonName ButtonNames[COLECO_BUTTON_NAME_COUNT];

/*
 * Copies the name of the button with the specified value into the
 * buffer
 *
 * buff       The buffer to copy the name into
 * buffsize   The size of the buffer
 * value      The value of the button
 * entry      The database entry
 */
extern void wii_coleco_db_get_button_name( 
    char* buff, int buffsize, u32 value, ColecoDBEntry* entry );

/*
 * Returns the index of the button in the list of mappable buttons
 *
 * value    The button value;
 * return   The index of the button in the list of mappable buttons
 */
extern int wii_coleco_db_get_button_index( u32 value );

/*
 * Returns the database entry for the game with the specified hash
 *
 * hash	  The hash of the game
 * entry  The entry to populate for the specified game
 */
extern void wii_coleco_db_get_entry( char* hash, ColecoDBEntry* entry );

/*
 * Writes the specified entry to the database for the game with the specified
 * hash.
 *
 * hash		The hash of the game
 * entry	The entry to write to the database
 * return	Whether the write was successful
 */
extern int wii_coleco_db_write_entry( char* hash, ColecoDBEntry *entry );

/*
 * Deletes the entry from the database with the specified hash
 *
 * hash		The hash of the game
 * return	Whether the delete was successful
 */
extern int wii_coleco_db_delete_entry( char* hash );

/*
 * Populates the specified entry with default values. The default values
 * are based on the "controlsMode" in the specified entry.
 *
 * entry      The entry to populate with default values
 * fullClear  Whether to fully clear the entry
 */
extern void wii_coleco_db_get_defaults( 
  ColecoDBEntry* entry, BOOL fullClear );

/*
 * Returns whether the button at the specified index is available for
 * the specified mode.
 *
 * index    The button index
 * mode     The controller mode
 * return   Whether the button is available for the specified mode
 */
extern int wii_coleco_db_is_button_available( u16 index, u8 mode );


/* 
 * Returns the description for the button with the specified value
 *
 * value    The button value
 * entry    The entry containing the button descriptions
 * return   The description for the button with the specified value
 */
extern char* wii_coleco_db_get_button_description( 
  u32 value, ColecoDBEntry *entry );

#endif
