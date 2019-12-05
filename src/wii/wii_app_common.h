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

#ifndef WII_APP_COMMON_H
#define WII_APP_COMMON_H

#include "wii_main.h"

#define WII_FILES_DIR "/wiicolem/"
#define WII_ROMS_DIR WII_FILES_DIR "roms/"
#define WII_SAVES_DIR WII_FILES_DIR "saves/"
#define WII_STATES_DIR WII_FILES_DIR "states/"
#define WII_OVERLAYS_DIR WII_FILES_DIR "overlays/"
#define WII_CONFIG_FILE WII_FILES_DIR "wiicolem.conf"

#define WII_BASE_APP_DIR "sd:/apps/wiicolem/"

#define WII_SAVE_GAME_EXT "sta"
#define WII_STATE_GAME_EXT "sav"
#define WII_COLECO_ROM WII_FILES_DIR "COLECO.ROM"

/*
 * The different types of nodes in the menu
 */
enum NODETYPE
{
  NODETYPE_ROOT = 0,
  NODETYPE_SPACER,
  NODETYPE_LOAD_ROM,
  NODETYPE_ROM,
  NODETYPE_EXIT,
  NODETYPE_RESUME,
  NODETYPE_ADVANCED,
  NODETYPE_SAVE_STATE_MANAGEMENT,
  NODETYPE_DEBUG_MODE,
  NODETYPE_TOP_MENU_EXIT,
  NODETYPE_AUTO_LOAD_STATE,
  NODETYPE_AUTO_SAVE_STATE,
  NODETYPE_RESET,
  NODETYPE_SAVE_STATE,
  NODETYPE_LOAD_STATE,
  NODETYPE_DELETE_STATE,
  NODETYPE_STATE_SAVE,
  NODETYPE_VSYNC,
  NODETYPE_DISPLAY_SETTINGS,
  NODETYPE_CONTROLS_SETTINGS,
  NODETYPE_PALETTE,
  NODETYPE_SHOW_ALL_SPRITES,
  NODETYPE_CARTRIDGE_SETTINGS_CURRENT,
  NODETYPE_CARTRIDGE_SETTINGS_CURRENT_SPACER,
  NODETYPE_CARTRIDGE_SETTINGS_DEFAULT,
  NODETYPE_VIEW_AS_CONTROLLER,
  NODETYPE_BUTTON1, NODETYPE_BUTTON2, NODETYPE_BUTTON3, NODETYPE_BUTTON4,
  NODETYPE_BUTTON5, NODETYPE_BUTTON6, NODETYPE_BUTTON7, NODETYPE_BUTTON8,
  NODETYPE_SAVE_CARTRIDGE_SETTINGS,
  NODETYPE_DELETE_CARTRIDGE_SETTINGS,
  NODETYPE_CONTROLS_MODE,
  NODETYPE_WIIMOTE_HORIZONTAL,
  NODETYPE_CARTRIDGE_SETTINGS_CONTROLS,
  NODETYPE_CARTRIDGE_SETTINGS_ADVANCED,
  NODETYPE_OPCODE_MEMORY,
  NODETYPE_CART_SRAM,
  NODETYPE_REVERT_CARTRIDGE_SETTINGS,
  NODETYPE_SPINNER,
  NODETYPE_SENSITIVITY,
  NODETYPE_KEYPAD_PAUSE,
  NODETYPE_KEYPAD_PAUSE_CART,
  NODETYPE_KEYPAD_SIZE,
  NODETYPE_KEYPAD_SIZE_CART,
  NODETYPE_USE_OVERLAY,
  NODETYPE_USE_OVERLAY_CART,
  NODETYPE_CYCLE_ADJUST,
  NODETYPE_VOLUME,
  NODETYPE_MAX_FRAMES,
  NODETYPE_RESIZE_SCREEN,
  NODETYPE_MAX_FRAMES_CART,
  NODETYPE_WIIMOTE_MENU_ORIENT,
  NODETYPE_SUPER_GAME_MODULE,
  NODETYPE_EEPROM,
  NODETYPE_16_9_CORRECTION,
  NODETYPE_FULL_WIDESCREEN,
  NODETYPE_FILTER,
  NODETYPE_GX_VI_SCALER
};

#endif
