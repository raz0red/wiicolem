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

#include <stdio.h>
#include <stdlib.h>

#include "Coleco.h"

#include "wii_app.h"
#include "wii_resize_screen.h"
#include "wii_sdl.h"
#include "wii_snapshot.h"
#include "wii_util.h"
#include "fileop.h"
#include "networkop.h"

#include "wii_coleco.h"
#include "wii_coleco_emulation.h"
#include "wii_coleco_snapshot.h"

#include "gettext.h"

extern "C" {
  void WII_VideoStart();
  void WII_VideoStop();
  void WII_SetDefaultVideoMode();
  int PauseAudio(int Switch);
  void ResetAudio();
}

// Whether we are loading a game
static BOOL loading_game = FALSE;
// Have we read the games list yet?
static BOOL games_read = FALSE;
// Whether we are pending a drive mount
static BOOL mount_pending = TRUE;
// The index of the last rom that was run
static s16 last_rom_index = 1;
// The roms node
static TREENODE *roms_menu;

// Forward refs
static void wii_read_game_list( TREENODE *menu );

/*
 * Initializes the ColecoVision menu
 */
void wii_coleco_menu_init()
{  
  //
  // The root menu
  //

  wii_menu_root = wii_create_tree_node( NODETYPE_ROOT, "root" );

  TREENODE* child = NULL;
  child = wii_create_tree_node( NODETYPE_RESUME, "Resume" );
  wii_add_child( wii_menu_root, child );

  child = NULL;
  child = wii_create_tree_node( NODETYPE_RESET, "Reset" );
  wii_add_child( wii_menu_root, child );

  child = wii_create_tree_node( NODETYPE_LOAD_ROM, "Load cartridge" );
  roms_menu = child;
  wii_add_child( wii_menu_root, child );

  //
  // Save state management
  //

  child = wii_create_tree_node( NODETYPE_CARTRIDGE_SETTINGS_CURRENT_SPACER, "" );
  wii_add_child( wii_menu_root, child );
  
  TREENODE* states = wii_create_tree_node( 
    NODETYPE_CARTRIDGE_SAVE_STATES, "Save states" );
  wii_add_child( wii_menu_root, states );

  child = wii_create_tree_node( 
    NODETYPE_CARTRIDGE_SAVE_STATES_SLOT, "Slot" );
  child->x = -2; child->value_x = -3;
  wii_add_child( states, child );

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( states, child );

  child = wii_create_tree_node( NODETYPE_SAVE_STATE, "Save state" );
  wii_add_child( states, child );

  child = wii_create_tree_node( NODETYPE_LOAD_STATE, "Load state" );
  wii_add_child( states, child );

  //
  // The cartridge settings (current) menu
  //   
  
  TREENODE *cart_settings = wii_create_tree_node( 
    NODETYPE_CARTRIDGE_SETTINGS_CURRENT, "Cartridge-specific settings" );
  wii_add_child( wii_menu_root, cart_settings );      

  // Controls sub-menu

  TREENODE *controls = wii_create_tree_node( 
    NODETYPE_CARTRIDGE_SETTINGS_CONTROLS, "Control settings" );                                                        
  wii_add_child( cart_settings, controls );

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( cart_settings, child );

  child = wii_create_tree_node( NODETYPE_CONTROLS_MODE, "Controller " );
  child->x = -2; child->value_x = -3;
  wii_add_child( controls, child );

  child = wii_create_tree_node( NODETYPE_SENSITIVITY, "Sensitivity " );
  child->x = -2; child->value_x = -3;
  wii_add_child( controls, child );  

  child = wii_create_tree_node( NODETYPE_SPINNER, "Spinner " );
  child->x = -2; child->value_x = -3;
  wii_add_child( controls, child );

  child = wii_create_tree_node( NODETYPE_WIIMOTE_HORIZONTAL, "Wiimote " );
  child->x = -2; child->value_x = -3;
  wii_add_child( controls, child );

  child = wii_create_tree_node( NODETYPE_VIEW_AS_CONTROLLER, "(View as) " );
  child->x = -2; child->value_x = -3;
  wii_add_child( controls, child );

  int button;
  for( button = NODETYPE_BUTTON1; button <= NODETYPE_BUTTON8; button++ )
  {
    child = wii_create_tree_node( (NODETYPE)button, "Button " );
    child->x = -2; child->value_x = -3;
    wii_add_child( controls, child );
  }

  // Advanced sub-menu

  TREENODE *cartadvanced = wii_create_tree_node( 
    NODETYPE_CARTRIDGE_SETTINGS_ADVANCED, "Advanced" );
  wii_add_child( cart_settings, cartadvanced );

  child = wii_create_tree_node( 
    NODETYPE_KEYPAD_PAUSE_CART, "Keypad pause " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );  

  child = wii_create_tree_node( 
    NODETYPE_KEYPAD_SIZE_CART, "Keypad size " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );  

  child = wii_create_tree_node( 
    NODETYPE_USE_OVERLAY_CART, "Keypad overlay " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );  

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( cartadvanced, child );

  child = wii_create_tree_node( 
    NODETYPE_CART_SRAM, "Battery-backed SRAM " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );

  child = wii_create_tree_node( 
    NODETYPE_OPCODE_MEMORY, "Opcode memory " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );
  
  child = wii_create_tree_node( 
    NODETYPE_EEPROM, "EEPROM " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );  

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( cartadvanced, child );

  child = wii_create_tree_node( NODETYPE_MAX_FRAMES_CART, 
    "Maximum frame rate " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );

  child = wii_create_tree_node( 
    NODETYPE_CYCLE_ADJUST, "Cycle adjust " );
  child->x = -2; child->value_x = -3;
  wii_add_child( cartadvanced, child );  

  // Save/Revert/Delete

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( cart_settings, child );

  child = wii_create_tree_node( 
    NODETYPE_SAVE_CARTRIDGE_SETTINGS, "Save settings" );
  wii_add_child( cart_settings, child );  

  child = wii_create_tree_node( 
    NODETYPE_REVERT_CARTRIDGE_SETTINGS, "Revert to saved settings" );
  wii_add_child( cart_settings, child );  

  child = wii_create_tree_node( 
    NODETYPE_DELETE_CARTRIDGE_SETTINGS, "Delete settings" );
  wii_add_child( cart_settings, child );  
  
  //
  // The advanced menu
  //
  
  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( wii_menu_root, child );

  TREENODE *advanced = wii_create_tree_node( NODETYPE_ADVANCED, 
    "Advanced" );
  wii_add_child( wii_menu_root, advanced );    
  

  //
  // The display settings menu
  //

  TREENODE *display = wii_create_tree_node( NODETYPE_DISPLAY_SETTINGS, 
    "Video settings" );
  wii_add_child( advanced, display );

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( advanced, child );

  child = wii_create_tree_node( NODETYPE_RESIZE_SCREEN, 
    "Screen size " );      
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );     

  child = wii_create_tree_node( NODETYPE_VSYNC, 
    "Vertical sync " );                        
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );   

  child = wii_create_tree_node( NODETYPE_MAX_FRAMES, 
    "Maximum frame rate " );
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );

  child = wii_create_tree_node( NODETYPE_FULL_WIDESCREEN, 
    "Full widescreen " );
  child->x = -2; child->value_x = -3;    
  wii_add_child( display, child );
  
  child = wii_create_tree_node( NODETYPE_16_9_CORRECTION, 
    "16:9 correction " );
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );  
    
  child = wii_create_tree_node( NODETYPE_GX_VI_SCALER, "Scaler " );
  child->x = -2; child->value_x = -3;    
  wii_add_child( display, child );  
  
  child = wii_create_tree_node( NODETYPE_FILTER, 
    "Bilinear filter " );
  child->x = -2; child->value_x = -3;    
  wii_add_child( display, child );    
  
  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( display, child );  
  
  child = wii_create_tree_node( NODETYPE_PALETTE, 
    "Palette " );                        
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );     

  child = wii_create_tree_node( NODETYPE_SHOW_ALL_SPRITES, 
    "Show all sprites " );                        
  child->x = -2; child->value_x = -3;
  wii_add_child( display, child );       

  child = wii_create_tree_node( 
    NODETYPE_KEYPAD_PAUSE, "Keypad pause " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );  

  child = wii_create_tree_node( 
    NODETYPE_KEYPAD_SIZE, "Keypad size " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );  

  child = wii_create_tree_node( 
    NODETYPE_USE_OVERLAY, "Keypad overlays " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );  
  
  child = wii_create_tree_node( 
    NODETYPE_SUPER_GAME_MODULE, "Super Game Module " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );    

  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( advanced, child );

  child = wii_create_tree_node( 
    NODETYPE_VOLUME, "Volume " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );  

  child = wii_create_tree_node( NODETYPE_TOP_MENU_EXIT, 
    "Top menu exit " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );

  child = wii_create_tree_node( NODETYPE_WIIMOTE_MENU_ORIENT, 
    "Wiimote (menu) " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );
  
  child = wii_create_tree_node( NODETYPE_SPACER, "" );
  wii_add_child( advanced, child );

  child = wii_create_tree_node( NODETYPE_DEBUG_MODE, 
    "Debug mode " );
  child->x = -2; child->value_x = -3;
  wii_add_child( advanced, child );  

  wii_menu_push( wii_menu_root );	
}

/*
 * Updates the buffer with the header message for the current menu
 *
 * menu     The menu
 * buffer   The buffer to update with the header message for the
 *          current menu.
 */
void wii_menu_handle_get_header( TREENODE* menu, char *buffer )
{
  if( loading_game )
  {
    snprintf( buffer, WII_MENU_BUFF_SIZE, gettextmsg("Loading game...") );
  }
  else
  {  
    switch( menu->node_type )
    {
      case NODETYPE_LOAD_ROM:
        if( !games_read )
        {
          snprintf( buffer, WII_MENU_BUFF_SIZE, 
              mount_pending ? 
                gettextmsg("Attempting to mount drive...") :
                gettextmsg("Reading game list...") );                
        }
        break;
      default:
        /* do nothing */
        break;
    }
  }
}


/*
 * Updates the buffer with the footer message for the current menu
 *
 * menu     The menu
 * buffer   The buffer to update with the footer message for the
 *          current menu.
 */
void wii_menu_handle_get_footer( TREENODE* menu, char *buffer )
{
  if( loading_game )
  {
    snprintf( buffer, WII_MENU_BUFF_SIZE, gettextmsg("Loading game...") );
  }
  else
  {
    switch( menu->node_type )
    {
      case NODETYPE_LOAD_ROM:
        if( games_read )
        {
          wii_get_list_footer( 
            roms_menu, "item", "items", buffer );
        }
        break;
      default:
        break;
    }
  }
}

/*
 * Updates the buffer with the name of the specified node
 *
 * node     The node
 * name     The name of the specified node
 * value    The value of the specified node
 */
void wii_menu_handle_get_node_name( 
  TREENODE* node, char *buffer, char* value )
{
  const char *strmode = NULL;
  int index;

  snprintf( buffer, WII_MENU_BUFF_SIZE, "%s", node->name );

  switch( node->node_type )
  {
    case NODETYPE_ROOT_DRIVE:
      {
        int device;
        FindDevice( node->name, &device );
        switch( device )
        {
          case DEVICE_SD:
            snprintf( buffer, WII_MENU_BUFF_SIZE, "[%s]", 
              "SD Card" );
            break;
          case DEVICE_USB:
            snprintf( buffer, WII_MENU_BUFF_SIZE, "[%s]",
              "USB Device" );
            break;
          case DEVICE_SMB:
            snprintf( buffer, WII_MENU_BUFF_SIZE, "[%s]",
              "Network Share" );
            break;
        }
      }
      break;  
    case NODETYPE_DIR:
      snprintf( buffer, WII_MENU_BUFF_SIZE, "[%s]", node->name );
      break;  
    case NODETYPE_CARTRIDGE_SAVE_STATES_SLOT:
      {
        BOOL isLatest;
        int current = wii_snapshot_current_index( &isLatest );
        current++;
        if( !isLatest )
        {
          snprintf( value, WII_MENU_BUFF_SIZE, "%d", current );
        }
        else
        {
          snprintf( value, WII_MENU_BUFF_SIZE, "%d (%s)", 
            current, gettextmsg( "Latest" ) );
        }
      }
      break;      
    case NODETYPE_RESIZE_SCREEN:
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", 
        ( ( wii_screen_x == DEFAULT_SCREEN_X && 
          wii_screen_y == DEFAULT_SCREEN_Y ) ? "(default)" : "Custom" ) );
      break;
    case NODETYPE_GX_VI_SCALER:
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", 
        ( wii_gx_vi_scaler ? "GX + VI" : "GX" ) );
      break;      
    case NODETYPE_FULL_WIDESCREEN:
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", 
        ( wii_full_widescreen == WS_AUTO ? "(auto)" :
          ( wii_full_widescreen ? "Enabled" : "Disabled" ) ) );
      break;      
    case NODETYPE_VOLUME:
      snprintf( value, WII_MENU_BUFF_SIZE, "%d", wii_volume );
      break;
    case NODETYPE_MAX_FRAMES:
      snprintf( value, WII_MENU_BUFF_SIZE, "%d", wii_max_frames );
      break;
    case NODETYPE_MAX_FRAMES_CART:
      if( wii_coleco_db_entry.maxFrames == MAX_FRAMES_DEFAULT )
      {
        snprintf( value, WII_MENU_BUFF_SIZE, "(default)" );
      }
      else
      {
        snprintf( value, WII_MENU_BUFF_SIZE, "%d", 
          wii_coleco_db_entry.maxFrames );
      }      
      break;
    case NODETYPE_DEBUG_MODE:
    case NODETYPE_TOP_MENU_EXIT:
    case NODETYPE_SHOW_ALL_SPRITES:
    case NODETYPE_CART_SRAM:
    case NODETYPE_OPCODE_MEMORY:
    case NODETYPE_VSYNC:
    case NODETYPE_SPINNER:
    case NODETYPE_KEYPAD_PAUSE:
    case NODETYPE_USE_OVERLAY:
    case NODETYPE_SUPER_GAME_MODULE:    
    case NODETYPE_16_9_CORRECTION:   
    case NODETYPE_FILTER: 
      {
        BOOL enabled = FALSE;
        switch( node->node_type )
        {
          case NODETYPE_FILTER:
            enabled = wii_filter;
            break;        
          case NODETYPE_16_9_CORRECTION:
            enabled = wii_16_9_correction;
            break;        
          case NODETYPE_SUPER_GAME_MODULE:    
            enabled = wii_super_game_module;
            break;
          case NODETYPE_SPINNER:
            enabled = !( wii_coleco_db_entry.flags&DISABLE_SPINNER );
            break;
          case NODETYPE_CART_SRAM:
            enabled = wii_coleco_db_entry.flags&CART_SRAM;
            break;
          case NODETYPE_OPCODE_MEMORY:
            enabled = wii_coleco_db_entry.flags&OPCODE_MEMORY;
            break;
          case NODETYPE_SHOW_ALL_SPRITES:
            enabled = wii_coleco_mode&CV_ALLSPRITE;
            break;
          case NODETYPE_VSYNC:
            enabled = ( wii_vsync == VSYNC_ENABLED );
            break;
          case NODETYPE_DEBUG_MODE:
            enabled = wii_debug;
            break;
          case NODETYPE_TOP_MENU_EXIT:
            enabled = wii_top_menu_exit;
            break;
          case NODETYPE_KEYPAD_PAUSE:
            enabled = wii_keypad_pause;
            break;
          case NODETYPE_USE_OVERLAY:
            enabled = wii_use_overlay;
            break;
          default:
            /* do nothing */
            break;
        }
        snprintf( value, WII_MENU_BUFF_SIZE, "%s", 
          enabled ? "Enabled" : "Disabled" );
        break;
    }
    case NODETYPE_PALETTE:      
      switch( wii_coleco_mode&CV_PALETTE )
      {
        case CV_PALETTE0:      
          strmode="Scaled VDP";
          break;
        case CV_PALETTE1:
          strmode="Original VDP";
          break;
        default:
          strmode="Faded NTSC";
          break;        
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;
    case NODETYPE_KEYPAD_SIZE:
    case NODETYPE_KEYPAD_SIZE_CART:
      {
        int size = 
          ( node->node_type == NODETYPE_KEYPAD_SIZE_CART ?
              wii_coleco_db_entry.keypadSize :
              wii_keypad_size );

        switch( size )
        {
          case KEYPAD_SIZE_DEFAULT:      
            strmode="(default)";
            break;
          case KEYPAD_SIZE_LARGE:
            strmode="Large";
            break;
          case KEYPAD_SIZE_MED:
            strmode="Medium";
            break;
          default:
            strmode="Small";
            break;        
        }          
        snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      }
      break;
    case NODETYPE_VIEW_AS_CONTROLLER:
      snprintf( value, WII_MENU_BUFF_SIZE, "%s",
        WiiControllerNames[wii_coleco_controller_view_mode] );
      break;
    case NODETYPE_BUTTON1:
    case NODETYPE_BUTTON2:
    case NODETYPE_BUTTON3:
    case NODETYPE_BUTTON4:
    case NODETYPE_BUTTON5:
    case NODETYPE_BUTTON6:
    case NODETYPE_BUTTON7:
    case NODETYPE_BUTTON8:
      {
        char btnName[255];
        index = ( node->node_type - NODETYPE_BUTTON1 );
        wii_coleco_db_get_button_name(
          btnName, sizeof( btnName ),
          wii_coleco_db_entry.button[index], 
          &wii_coleco_db_entry );
        snprintf( buffer, WII_MENU_BUFF_SIZE, "%s ",
          WiiButtonDescriptions[index][wii_coleco_controller_view_mode] );
        snprintf( value, WII_MENU_BUFF_SIZE, "%s", btnName );
      }
      break;     
    case NODETYPE_CONTROLS_MODE:
      switch( wii_coleco_db_entry.controlsMode )
      {
        case CONTROLS_MODE_SUPERACTION:      
          strmode="Super Action";
          break;
        case CONTROLS_MODE_ROLLER:
          strmode="Roller";
          break;
        case CONTROLS_MODE_DRIVING:
          strmode="Driving (Analog)";
          break;
        case CONTROLS_MODE_DRIVING_TILT:
          strmode="Driving (Wiimote/Tilt)";
          break;
        default:
          strmode="Standard";
          break;        
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;
    case NODETYPE_WIIMOTE_MENU_ORIENT:
      if( wii_mote_menu_vertical )
      {
        strmode="Upright";
      }
      else
      {
        strmode="Sideways";
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;
    case NODETYPE_WIIMOTE_HORIZONTAL:
      if( wii_coleco_db_entry.wiiMoteHorizontal )
      {
        strmode="Sideways";
      }
      else
      {
        strmode="Upright";
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;      
    case NODETYPE_SENSITIVITY:
      {
        int val = ( 5 - wii_coleco_db_entry.sensitivity  );
        snprintf( value, WII_MENU_BUFF_SIZE, "%s%d", 
          ( val < 0 ? "-" : ( val == 0 ? "" : "+" ) ),
          ( val < 0 ? -val : val ) );
      }
      break;
    case NODETYPE_KEYPAD_PAUSE_CART:
      switch( wii_coleco_db_entry.keypadPause )
      {
        case KEYPAD_PAUSE_ENABLED:
          strmode = "Enabled";
          break;
        case KEYPAD_PAUSE_DISABLED:
          strmode = "Disabled";
          break;
        default:
          strmode = "(default)";
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;
    case NODETYPE_USE_OVERLAY_CART:
      switch( wii_coleco_db_entry.overlayMode )
      {
        case OVERLAY_ENABLED:
          strmode = "Enabled";
          break;
        case OVERLAY_DISABLED:
          strmode = "Disabled";
          break;
        default:
          strmode = "(default)";
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;
    case NODETYPE_EEPROM:
      switch( wii_coleco_db_entry.eeprom )
      {
        case EEPROM_24C08:
          strmode = "24c08";
          break;
        case EEPROM_24C256:
          strmode = "24c256";
          break;
        default:
          strmode = "None";
      }
      snprintf( value, WII_MENU_BUFF_SIZE, "%s", strmode );
      break;      
    case NODETYPE_CYCLE_ADJUST:
      snprintf( value, WII_MENU_BUFF_SIZE, "%d", wii_coleco_db_entry.cycleAdjust );
      break;
    default:
      break;
  }
}

/*
 * React to the "select" event for the specified node
 *
 * node     The node
 */
void wii_menu_handle_select_node( TREENODE *node )
{
  char buff[WII_MAX_PATH];

  switch( node->node_type )
  {
    case NODETYPE_RESIZE_SCREEN:
      {
        wii_resize_screen_draw_border( blit_surface, 0, COLECO_HEIGHT );
        wii_sdl_put_image_normal( 1 );
        wii_sdl_flip(); 
        resize_info rinfo = { 
          (float)DEFAULT_SCREEN_X, 
          (float)DEFAULT_SCREEN_Y, 
          (float)wii_screen_x, 
          (float)wii_screen_y };
        wii_resize_screen_gui( &rinfo );
        wii_screen_x = rinfo.currentX;
        wii_screen_y = rinfo.currentY;
      }
      break;
    case NODETYPE_FULL_WIDESCREEN:
      wii_full_widescreen++;
      if( wii_full_widescreen > WS_AUTO )
      {
        wii_full_widescreen = 0;
      }
      wii_update_widescreen();
      break;      
    case NODETYPE_VOLUME:
      wii_volume++;
      if( wii_volume > 15 )
      {
        wii_volume = 0;
      }
      break;
    case NODETYPE_SAVE_STATE:
      wii_save_snapshot( NULL, TRUE );
      break;
    case NODETYPE_CARTRIDGE_SAVE_STATES_SLOT:
      wii_snapshot_next();
      break;      
    case NODETYPE_MAX_FRAMES:
      ++wii_max_frames;
      if( wii_max_frames > 80 )
      {
        wii_max_frames = 30;
      }
      break;
    case NODETYPE_MAX_FRAMES_CART:
      ++wii_coleco_db_entry.maxFrames;
      if( wii_coleco_db_entry.maxFrames > 80 )
      {
        wii_coleco_db_entry.maxFrames = MAX_FRAMES_DEFAULT;
      }
      else if( wii_coleco_db_entry.maxFrames < 30 )
      {
        wii_coleco_db_entry.maxFrames = 30;
      }
      break;
    case NODETYPE_EEPROM:
      wii_coleco_db_entry.eeprom++;
      if( wii_coleco_db_entry.eeprom > EEPROM_24C256 )
      {
        wii_coleco_db_entry.eeprom = 0;
      }
      break;      
    case NODETYPE_CYCLE_ADJUST:
      wii_coleco_db_entry.cycleAdjust++;
      if( wii_coleco_db_entry.cycleAdjust == 51 )
      {
        wii_coleco_db_entry.cycleAdjust = -50;
      }
      break;
    case NODETYPE_KEYPAD_PAUSE_CART:
      wii_coleco_db_entry.keypadPause++;
      if( wii_coleco_db_entry.keypadPause > KEYPAD_PAUSE_DISABLED )
      {
        wii_coleco_db_entry.keypadPause = 0;
      }
      break;
    case NODETYPE_KEYPAD_SIZE_CART:
      wii_coleco_db_entry.keypadSize++;
      if( wii_coleco_db_entry.keypadSize > KEYPAD_SIZE_LARGE )
      {
        wii_coleco_db_entry.keypadSize = KEYPAD_SIZE_DEFAULT;
      }
      break;
    case NODETYPE_USE_OVERLAY_CART:
      wii_coleco_db_entry.overlayMode++;
      if( wii_coleco_db_entry.overlayMode > OVERLAY_DISABLED )
      {
        wii_coleco_db_entry.overlayMode = 0;
      }
      break;
    case NODETYPE_VSYNC:
      wii_set_vsync( wii_vsync ^ 1 );
      break;
    case NODETYPE_ROM:            
      snprintf( 
        buff, sizeof(buff), "%s%s", wii_get_roms_dir(), node->name );             
      loading_game = TRUE;
      if( wii_start_emulation( buff, NULL, FALSE, FALSE ) )
      {
        last_rom_index = wii_menu_get_current_index();
        ResetAudio();
        wii_menu_quit_loop = 1;
      }
      loading_game = FALSE;
      break;
    case NODETYPE_RESUME:
      if( wii_resume_emulation() )
      {
        wii_menu_quit_loop = 1;
      }
      break;
    case NODETYPE_RESET:
      if( wii_reset_emulation() )
      {
        ResetAudio();
        wii_menu_quit_loop = 1;
      }
      break;
    case NODETYPE_TOP_MENU_EXIT:
      wii_top_menu_exit ^= 1;
      break;
    case NODETYPE_WIIMOTE_MENU_ORIENT:
      wii_mote_menu_vertical ^= 1;
      break;
    case NODETYPE_DEBUG_MODE:
      wii_debug ^= 1;
      break;
    case NODETYPE_SUPER_GAME_MODULE:
      wii_super_game_module ^= 1;
      break;
    case NODETYPE_16_9_CORRECTION:
      wii_16_9_correction ^= 1;
      break;      
    case NODETYPE_FILTER:
      wii_filter ^= 1;
      break;
      case NODETYPE_GX_VI_SCALER:
        wii_gx_vi_scaler ^= 1;
        break;              
    case NODETYPE_KEYPAD_PAUSE:
      wii_keypad_pause ^= 1;
      break;      
    case NODETYPE_KEYPAD_SIZE:
      wii_keypad_size++;
      if( wii_keypad_size > KEYPAD_SIZE_LARGE )
      {
        wii_keypad_size = KEYPAD_SIZE_SMALL;
      }
      break;
    case NODETYPE_USE_OVERLAY:
      wii_use_overlay ^= 1;
      break;
    case NODETYPE_ROOT_DRIVE:
    case NODETYPE_UPDIR:
    case NODETYPE_DIR:
      if( node->node_type == NODETYPE_ROOT_DRIVE )
      {
        char path[WII_MAX_PATH];
        snprintf( path, sizeof(path), "%s/", node->name );
        wii_set_roms_dir( path );
        mount_pending = TRUE;
      }
      else if( node->node_type == NODETYPE_UPDIR )
      {
        const char* romsDir = wii_get_roms_dir();
        int len = strlen( romsDir );
        if( len > 1 && romsDir[len-1] == '/' )
        {
          char dirpart[WII_MAX_PATH] = "";
          char filepart[WII_MAX_PATH] = "";
          Util_splitpath( romsDir, dirpart, filepart );
          len = strlen(dirpart);
          if( len > 0 )
          {
            dirpart[len] = '/';
            dirpart[len+1] = '\0';
          }
          wii_set_roms_dir( dirpart );
        }
      }
      else
      {
        char newDir[WII_MAX_PATH];
        snprintf( newDir, sizeof(newDir), "%s%s/", 
          wii_get_roms_dir(), node->name );
        wii_set_roms_dir( newDir );
      }
      games_read = FALSE;
      last_rom_index = 1;
      break;            
    case NODETYPE_ADVANCED:
    case NODETYPE_LOAD_ROM:     
    case NODETYPE_DISPLAY_SETTINGS:
    case NODETYPE_CARTRIDGE_SETTINGS_CURRENT:
    case NODETYPE_CARTRIDGE_SETTINGS_CONTROLS:
    case NODETYPE_CARTRIDGE_SETTINGS_ADVANCED:
    case NODETYPE_CARTRIDGE_SAVE_STATES:
      wii_menu_push( node );
      if( node->node_type == NODETYPE_LOAD_ROM )
      {
        if (games_read ) 
        {
          wii_menu_move( node, last_rom_index );
        }
      }
      break;
    case NODETYPE_LOAD_STATE:
      if( wii_start_snapshot() )
      {
        ResetAudio();
        wii_menu_quit_loop = 1;
      }
      else
      {
        // Exit the save states (rom is no longer valid)
        wii_menu_pop();
      }
      break;
    case NODETYPE_PALETTE:      
      switch( wii_coleco_mode&CV_PALETTE )
      {
        case CV_PALETTE0:      
          wii_coleco_mode=((wii_coleco_mode&~CV_PALETTE)|CV_PALETTE1);
          break;
        case CV_PALETTE1:
          wii_coleco_mode=((wii_coleco_mode&~CV_PALETTE)|CV_PALETTE2);
          break;
        default:
          wii_coleco_mode=((wii_coleco_mode&~CV_PALETTE)|CV_PALETTE0);        
          break;
      }
      break;
    case NODETYPE_SHOW_ALL_SPRITES:
      if( wii_coleco_mode&CV_ALLSPRITE )
      {
        wii_coleco_mode&=~CV_ALLSPRITE;
      }
      else
      {
        wii_coleco_mode|=CV_ALLSPRITE;
      }
      break;
    case NODETYPE_VIEW_AS_CONTROLLER:
      ++wii_coleco_controller_view_mode;
      if( wii_coleco_controller_view_mode == CONTROLLER_NAME_COUNT )
      {
        wii_coleco_controller_view_mode = 0;
      }
      break;
    case NODETYPE_BUTTON1:
    case NODETYPE_BUTTON2:
    case NODETYPE_BUTTON3:
    case NODETYPE_BUTTON4:
    case NODETYPE_BUTTON5:
    case NODETYPE_BUTTON6:
    case NODETYPE_BUTTON7:
    case NODETYPE_BUTTON8:
      {
        int index = ( node->node_type - NODETYPE_BUTTON1 );      
        int btnindex = 
          wii_coleco_db_get_button_index(          
            wii_coleco_db_entry.button[index] );

        while( 1 )
        {
          btnindex++;
          if( btnindex == COLECO_BUTTON_NAME_COUNT )
          {
            btnindex = 0;
          }

          if( wii_coleco_db_is_button_available(
              btnindex, wii_coleco_db_entry.controlsMode ) )
          {
            break;
          }
        }
        wii_coleco_db_entry.button[index] = ButtonNames[btnindex].button;
      }
      break;     
    case NODETYPE_SAVE_CARTRIDGE_SETTINGS:
      if( wii_coleco_db_entry.name[0] == '\0' )
      {
        char cartname[WII_MAX_PATH];        
        Util_splitpath( wii_last_rom, NULL, cartname );
        char *ptr = strrchr( cartname, '.' );
        if( ptr ) *ptr = '\0';
        Util_strlcpy( 
          wii_coleco_db_entry.name, cartname, 
          sizeof( wii_coleco_db_entry.name ) );
      }
      if( wii_coleco_db_write_entry( 
        wii_cartridge_hash, &wii_coleco_db_entry ) )
      {
        wii_coleco_db_entry.loaded = 1;
        wii_set_status_message( "Successfully saved cartridge settings." );
      }
      else
      {
        wii_set_status_message( 
          "An error occurred saving cartridge settings." );
      }
      break;
    case NODETYPE_DELETE_CARTRIDGE_SETTINGS:
      if( wii_coleco_db_delete_entry( wii_cartridge_hash ) )
      {
        wii_menu_reset_indexes();
        wii_menu_move( wii_menu_stack[wii_menu_stack_head], 1 );
        wii_set_status_message( "Successfully deleted cartridge settings." );
      }
      else
      {
        wii_set_status_message( 
          "An error occurred deleting cartridge settings." );
      }
      // Load the values for the entry
      wii_coleco_db_get_entry( wii_cartridge_hash, &wii_coleco_db_entry );
      break;
    case NODETYPE_REVERT_CARTRIDGE_SETTINGS:
      wii_coleco_db_get_entry( wii_cartridge_hash, &wii_coleco_db_entry );
      wii_set_status_message( "Successfully reverted to saved settings." );
      break;
    case NODETYPE_CONTROLS_MODE:
      wii_coleco_db_entry.controlsMode++;
      if( wii_coleco_db_entry.controlsMode > CONTROLS_MODE_ROLLER )
      {
        wii_coleco_db_entry.controlsMode = CONTROLS_MODE_STANDARD;
      }
      wii_coleco_db_get_defaults( &wii_coleco_db_entry, FALSE );
      break;
    case NODETYPE_WIIMOTE_HORIZONTAL:
      wii_coleco_db_entry.wiiMoteHorizontal ^= 1;
      break;
    case NODETYPE_OPCODE_MEMORY:
      wii_coleco_db_entry.flags ^= OPCODE_MEMORY;
      break;
    case NODETYPE_CART_SRAM:
      wii_coleco_db_entry.flags ^= CART_SRAM;
      break;
    case NODETYPE_SPINNER:
      wii_coleco_db_entry.flags ^= DISABLE_SPINNER;
      break;
    case NODETYPE_SENSITIVITY:
      wii_coleco_db_entry.sensitivity--;
      if( wii_coleco_db_entry.sensitivity < 1 )
      {
        wii_coleco_db_entry.sensitivity = 9;
      }
      break;
    default:
      /* do nothing */
      break;
  }
}

/*
 * Determines whether the node is currently visible
 *
 * node     The node
 * return   Whether the node is visible
 */
BOOL wii_menu_handle_is_node_visible( TREENODE *node )
{
  switch( node->node_type )
  {
    case NODETYPE_LOAD_STATE:
      return wii_snapshot_current_exists();  
    case NODETYPE_RESET:
    case NODETYPE_RESUME:
    case NODETYPE_CARTRIDGE_SETTINGS_CURRENT:
    case NODETYPE_CARTRIDGE_SETTINGS_CURRENT_SPACER:
    case NODETYPE_CARTRIDGE_SAVE_STATES:
      return wii_last_rom != NULL;
    case NODETYPE_DELETE_CARTRIDGE_SETTINGS:
    case NODETYPE_REVERT_CARTRIDGE_SETTINGS:
      return wii_last_rom != NULL && wii_coleco_db_entry.loaded;
    case NODETYPE_SPINNER:
      return wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION;
    case NODETYPE_FILTER:
      return !wii_gx_vi_scaler;
    case NODETYPE_KEYPAD_PAUSE:
    case NODETYPE_KEYPAD_PAUSE_CART:
      return !wii_gx_vi_scaler;
    case NODETYPE_16_9_CORRECTION:
      return is_widescreen;
    case NODETYPE_SENSITIVITY:
      return 
        ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_ROLLER ) ||
        ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING ) ||
        ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING_TILT );
    case NODETYPE_USE_OVERLAY_CART:
      return wii_coleco_db_entry.overlay.keypOverlay[0] != '\0';
    default:
      /* do nothing */
      break;
  }

  return TRUE;
}

/*
 * Determines whether the node is selectable
 *
 * node     The node
 * return   Whether the node is selectable
 */
BOOL wii_menu_handle_is_node_selectable( TREENODE *node )
{
  if( node->node_type == NODETYPE_CARTRIDGE_SETTINGS_CURRENT_SPACER )
  {
    return FALSE;
  }

  return TRUE;
}

/*
 * Provides an opportunity for the specified menu to do something during 
 * a display cycle.
 *
 * menu     The menu
 */
void wii_menu_handle_update( TREENODE *menu )
{
  switch( menu->node_type )
  {
    case NODETYPE_LOAD_ROM:
      if( !games_read )
      {
        if( mount_pending )
        {
          const char* roms = wii_get_roms_dir();
          if( strlen( roms ) > 0 )
          {
            char mount[WII_MAX_PATH];
            Util_strlcpy( mount, roms, sizeof(mount) );

            resetSmbErrorMessage(); // Reset the SMB error message
            if( !ChangeInterface( mount, FS_RETRY_COUNT ) )
            {
              wii_set_roms_dir( "" );
              const char* netMsg = getSmbErrorMessage();
              if( netMsg != NULL )
              {
                wii_set_status_message( netMsg );
              }
              else
              {
                char msg[256];
                snprintf( msg, sizeof(msg), "%s: %s", 
                  "Unable to mount", mount );
                wii_set_status_message( msg );
              }
            }
          }
          mount_pending = FALSE;
        }

        wii_read_game_list( roms_menu );  
        wii_menu_reset_indexes();    
        wii_menu_move( roms_menu, 1 );
        wii_menu_force_redraw = 1;
#if 0      
        wii_read_game_list( menu );  
        wii_menu_reset_indexes();    
        wii_menu_move( menu, 1 );
        wii_menu_force_redraw = 1;
#endif        
      }
      break;
    default:
      /* do nothing */
      break;
  }
}

/*
* Used for comparing menu names when sorting (qsort)
*
* a        The first tree node to compare
* b        The second tree node to compare
* return   The result of the comparison
*/
static int game_name_compare( const void *a, const void *b )
{
  TREENODE** aptr = (TREENODE**)a;
  TREENODE** bptr = (TREENODE**)b;
  int type = (*aptr)->node_type - (*bptr)->node_type;
  return type != 0 ? type : strcasecmp( (*aptr)->name, (*bptr)->name );
}

/*
 * Reads the list of games into the specified menu
 *
 * menu     The menu to read the games into
 */
static void wii_read_game_list( TREENODE *menu )
{
  const char* roms = wii_get_roms_dir();

  wii_menu_clear_children( menu ); // Clear the children

#ifdef WII_NETTRACE
net_print_string( NULL, 0, "ReadGameList: %s\n", roms, strlen(roms) );
#endif

  BOOL success = FALSE;
  if( strlen( roms ) > 0 )
  {
    DIR *romdir = opendir( roms );

#ifdef WII_NETTRACE
net_print_string( NULL, 0, "OpenDir: %d\n", roms, romdir);
#endif

    if( romdir != NULL)
    {
      wii_add_child(
        menu, wii_create_tree_node( NODETYPE_UPDIR, "[..]" ) );

      struct dirent* entry = NULL;
      while( ( entry = readdir( romdir ) ) != NULL )
      {               
        if( ( strcmp( ".", entry->d_name ) && strcmp( "..", entry->d_name ) ) )
        {				                
          TREENODE *child = 
            wii_create_tree_node( 
              ( entry->d_type == DT_DIR ? NODETYPE_DIR : NODETYPE_ROM ), 
              entry->d_name );

          wii_add_child( menu, child );
        }
      }
      closedir( romdir );

      // Sort the games list
      qsort( menu->children, menu->child_count, 
        sizeof(*(menu->children)), game_name_compare );

      success = TRUE;
    }
    else
    {
      char msg[256];
      snprintf( msg, sizeof(msg), "%s: %s", 
        "Error opening", roms );
      wii_set_status_message( msg );
    }
  }

  if( !success )
  {
    wii_set_roms_dir( "" );
    wii_add_child( menu, 
      wii_create_tree_node( NODETYPE_ROOT_DRIVE, "sd:" ) );
    wii_add_child( menu, 
      wii_create_tree_node( NODETYPE_ROOT_DRIVE, "usb:" ) );
    wii_add_child( menu, 
      wii_create_tree_node( NODETYPE_ROOT_DRIVE, "smb:" ) );
  }
  
  games_read = TRUE;
}

/*
 * Invoked after exiting the menu loop
 */
void wii_menu_handle_post_loop()
{
  if( !ExitNow )
  {        
    // Resume SDL Video
    WII_VideoStart();

    // Start the sound
    PauseAudio(0);
  }
}

/*
 * Invoked prior to entering the menu loop
 */
void wii_menu_handle_pre_loop()
{
  // Autosave, etc.
  if( wii_last_rom != NULL )
  {
    if( wii_top_menu_exit )
    {
      // Pop to the top
      while( wii_menu_pop() != NULL );
    }
  }

  // Stop SDL video
  WII_VideoStop();
  WII_SetDefaultVideoMode();


  // Stop the sound
  PauseAudio(1);
}

/*
 * Invoked when the home button is pressed when the 
 * menu is being displayed
 */
void wii_menu_handle_home_button()
{
  ExitNow = true;
}