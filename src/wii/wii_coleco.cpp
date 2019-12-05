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

#include "wii_app.h"
#include "wii_config.h"
#include "wii_input.h"
#include "wii_sdl.h"

#include "wii_coleco.h"
#include "wii_coleco_keypad.h"
#include "wii_coleco_menu.h"

#include "FreeTypeGX.h"

#include "font_ttf.h"

extern "C" {
  void WII_SetWidescreen(int wide);
  void WII_VideoStop();
  void WII_VideoStart();
  void WII_ChangeSquare(int xscale, int yscale, int xshift, int yshift);
  void WII_SetFilter( BOOL filter );
  void WII_SetDefaultVideoMode();
  void WII_SetStandardVideoMode( int xscale, int yscale, int width );
}

// The last ColecoVision cartridge hash
char wii_cartridge_hash[33];
// The ColecoVision database entry for current game
ColecoDBEntry wii_coleco_db_entry;
// The ColecoVision Mode
int wii_coleco_mode;
// The coleco controller view mode
int wii_coleco_controller_view_mode = 0;
// Whether to display debug info (FPS, etc.)
short wii_debug = 0;
// Auto save state?
BOOL wii_auto_save_state = FALSE;
// Auto load state?
BOOL wii_auto_load_state = TRUE;
// Keypad pause
BOOL wii_keypad_pause = TRUE;
// Keypad size
u8 wii_keypad_size = KEYPAD_SIZE_MED;
// Overlay mode
BOOL wii_use_overlay = TRUE;
// Super Game Module Enabled
BOOL wii_super_game_module = TRUE;
// Volume
u8 wii_volume = 7;
// Maximum frame rate
u8 wii_max_frames = 60;
// The screen X size
int wii_screen_x = DEFAULT_SCREEN_X;
// The screen Y size
int wii_screen_y = DEFAULT_SCREEN_Y;
// Whether to filter the display
BOOL wii_filter = FALSE; 
// Whether to use the GX/VI scaler
BOOL wii_gx_vi_scaler = TRUE;
// The roms dir
static char roms_dir[WII_MAX_PATH] = "";

/*
 * Initializes the application
 */
void wii_handle_init()
{  
  wii_read_config();

  // Startup the SDL
  if( !wii_sdl_init() ) 
  {
    fprintf( stderr, "FAILED : Unable to init SDL: %s\n", SDL_GetError() );
    exit( EXIT_FAILURE );
  }

  // FreeTypeGX
  InitFreeType( (uint8_t*)font_ttf, (FT_Long)font_ttf_size  );
  
  wii_update_widescreen();

  // Initialize the colecovision menu
  wii_coleco_menu_init();
}

/*
 * Updates the widescreen mode
 */
void wii_update_widescreen() 
{
  int ws = 
    ( wii_full_widescreen == WS_AUTO ? 
        is_widescreen : wii_full_widescreen );
  WII_SetWidescreen( ws );
}

/*
 * Returns the screen size
 *
 * inX  Input x
 * inY  Input y
 * x    Out x
 * y    Out y 
 */
extern "C" void wii_get_screen_size( int inX, int inY, int *x, int *y )
{
    int xs = inX;
    int ys = inY;      
    if( is_widescreen && wii_16_9_correction )
    {
      xs = (xs*3)/4; // Widescreen correct
    }            
    xs = ((xs+1)&~1);
    ys = ((ys+1)&~1);      
    
    *x = xs;
    *y = ys;
}

/*
 * Sets the video mode
 *
 * allowVi  Whether to allow the GX+VI mode
 */
void wii_set_video_mode(BOOL allowVi) 
{
    // Get the screen size
    int x, y;
    wii_get_screen_size(wii_screen_x, wii_screen_y, &x, &y);
    
    if( allowVi && wii_gx_vi_scaler && !wii_filter )
    {
      // VI+GX
      WII_SetStandardVideoMode( x, y, TMS9918_WIDTH );      
    }
    else
    {
      // Scale the screen (GX)
      WII_SetDefaultVideoMode();
      WII_SetFilter( wii_filter );      
      WII_ChangeSquare( x, y, 0, 0 );
    }
}

/*
 * Frees resources prior to the application exiting
 */
void wii_handle_free_resources()
{
  wii_sdl_free_resources();
  wii_keypad_free_resources();
  SDL_Quit();
}

/*
 * Runs the application (main loop)
 */
void wii_handle_run()
{
  /* ColEm settings */
  Verbose=0;
  UPeriod=100;

  // Initialize and run ColEm
  if( InitMachine() )
  {
    StartColeco( NULL );
        
    // Clear the screen prior to exiting
    wii_sdl_black_screen();
    if (wii_gx_vi_scaler) {
      WII_VideoStart();
      wii_set_video_mode(TRUE);
      WII_VideoStop();
    }        
        
    TrashColeco();
    TrashMachine();
  }

  // We be done, write the config settings, free resources and exit
  wii_write_config();
}

#if 0
/*
 * Returns the roms directory
 *
 * return   The roms directory
 */
char* wii_get_roms_dir()
{
  if( roms_dir[0] == '\0' )
  {
    snprintf( roms_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(), WII_ROMS_DIR );
  }

  return roms_dir;
}
#endif

/*
 * Returns the roms directory
 *
 * return   The roms directory
 */
char* wii_get_roms_dir()
{
  return roms_dir;
}

/*
 * Sets the current rom directory
 *
 * newDir   The new roms directory
 */
void wii_set_roms_dir( const char* newDir )
{
  Util_strlcpy( roms_dir, newDir, sizeof(roms_dir) );
#ifdef WII_NETTRACE
  net_print_string( NULL, 0, "RomsDir: \"%s\"\n", roms_dir );
#endif
}

// The saves dir
static char saves_dir[WII_MAX_PATH] = "";

/*
 * Returns the saves directory
 *
 * return   The saves directory
 */
char* wii_get_saves_dir()
{
  if( saves_dir[0] == '\0' )
  {
    snprintf( saves_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(), WII_SAVES_DIR );
  }

  return saves_dir;
}

// The states dir
static char states_dir[WII_MAX_PATH] = "";

/*
 * Returns the states directory
 *
 * return   The states directory
 */
char* wii_get_states_dir()
{
  if( states_dir[0] == '\0' )
  {
    snprintf( states_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(), WII_STATES_DIR );
  }

  return states_dir;
}


// The overlays dir
static char overlays_dir[WII_MAX_PATH] = "";

/*
 * Returns the overlays directory
 *
 * return   The overlays directory
 */
char* wii_get_overlays_dir()
{
  if( overlays_dir[0] == '\0' )
  {
    snprintf( overlays_dir, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(), WII_OVERLAYS_DIR );
  }

  return overlays_dir;
}

/*
 * Returns the button value for the specified button index
 *
 * current      The current statue of the joystick
 * buttonIndex  The button to return the value for
 */
static u32 get_button_value( u32 current, u8 buttonIndex )
{
  u32 val = wii_coleco_db_entry.button[buttonIndex];

  // If the value isn't related to the keypad, return it.
  // If it is related to the keypad, ensure no other keypad
  // button is currently pressed.
  return 
    ( !( val & JST_KEYPAD ) ? val :
      ( ( current & JST_KEYPAD ) ? 0 : val ) );
}

/*
 * Checks and returns the state of the specified joystick
 *
 * joyIndex   The index of the joystick to check
 * return     The current state of the specified joystick
 */
u32 wii_coleco_poll_joystick( int joyIndex )
{
  u32 I = 0;

  // Check the state of the controllers
  u32 held = WPAD_ButtonsHeld( joyIndex );
  u32 gcHeld = PAD_ButtonsHeld( joyIndex );

  expansion_t exp;
  WPAD_Expansion( joyIndex, &exp );
  u8 isClassic = ( exp.type == WPAD_EXP_CLASSIC );

  // Whether we allow input from the left analog joystick
  s8 allowAnalogLjs = 
    ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_STANDARD ||
      wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION );

  // Whether we allow input from the right analog joystick
  s8 allowAnalogRjs =
    ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_STANDARD ) ||
    ( ( wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION ) &&
      ( wii_coleco_db_entry.flags&DISABLE_SPINNER ) );    

  // Whether the Wiimote is horizontal
  u8 wmHorizontal = wii_coleco_db_entry.wiiMoteHorizontal;

  // Whether Keypad as D-Pad is enabled
  u8 kPadAsDpad = wii_coleco_db_entry.flags&KPAD_AS_DPAD;

  float expX = 0, expY = 0, expRjsX = 0, expRjsY = 0;
  s8 gcX = 0, gcY = 0, gcRjsX = 0, gcRjsY = 0;

  if( allowAnalogLjs )
  {
    if( isClassic )
    {
      expX = wii_exp_analog_val( &exp, TRUE, FALSE );
      expY = wii_exp_analog_val( &exp, FALSE, FALSE );
    }
    gcX = PAD_StickX( joyIndex );
    gcY = PAD_StickY( joyIndex );
  }

  if( allowAnalogRjs )
  {
    expRjsX = wii_exp_analog_val( &exp, TRUE, TRUE );
    expRjsY = wii_exp_analog_val( &exp, FALSE, TRUE );
    gcRjsX = PAD_SubStickX( joyIndex );
    gcRjsY = PAD_SubStickY( joyIndex );
  }

  // Right
  if( wii_digital_right( wmHorizontal, 1, held ) || 
      ( gcHeld & GC_BUTTON_CV_RIGHT ) ||
      ( wii_analog_right( expX, gcX ) ) || 
      ( !kPadAsDpad && wii_analog_right( expRjsX, gcRjsX ) ) )
  {
    I|=JST_RIGHT;
  }

  // Left
  if( wii_digital_left( wmHorizontal, 0, held ) ||
      ( isClassic ? ( held & WII_CLASSIC_CV_LEFT ) : 0 ) || 
      ( gcHeld & GC_BUTTON_CV_LEFT ) || 
      ( wii_analog_left( expX, gcX ) ) ||
      ( !kPadAsDpad && wii_analog_left( expRjsX, gcRjsX ) ) )
  {
    I|=JST_LEFT;
  }

  // Down
  if( wii_digital_down( wmHorizontal, 1, held ) || 
      ( gcHeld & GC_BUTTON_CV_DOWN ) || 
      ( wii_analog_down( expY, gcY ) ) || 
      ( !kPadAsDpad && wii_analog_down( expRjsY, gcRjsY ) ) )
  {
    I|=JST_DOWN;
  }

  // Up
  if( wii_digital_up( wmHorizontal, 0, held ) ||
      ( isClassic ? ( held & WII_CLASSIC_CV_UP ) : 0 ) || 
      ( gcHeld & GC_BUTTON_CV_UP ) || 
      ( wii_analog_up( expY, gcY ) ) ||
      ( !kPadAsDpad && wii_analog_up( expRjsY, gcRjsY ) ) )
  {
    I|=JST_UP;
  }

  // Button 1
  if( held & ( WII_BUTTON_CV_1 | 
    ( isClassic ? WII_CLASSIC_CV_1 : WII_NUNCHECK_CV_1 ) ) || 
    gcHeld & GC_BUTTON_CV_1 )
  {
    //I|=wii_coleco_db_entry.button[0];
    I|=get_button_value( I, 0 );
  }

  // Button 2
  if( held & ( WII_BUTTON_CV_2 | 
    ( isClassic ? WII_CLASSIC_CV_2 : WII_NUNCHECK_CV_2 ) ) || 
    gcHeld & GC_BUTTON_CV_2 )
  {
    //I|=wii_coleco_db_entry.button[1];
    I|=get_button_value( I, 1 );
  }

  // Button 3
  if( held & ( WII_BUTTON_CV_3 | WII_CLASSIC_CV_3 ) || 
      gcHeld & GC_BUTTON_CV_3 )
  {
    //I|=wii_coleco_db_entry.button[2];
    I|=get_button_value( I, 2 );
  }

  // Button 4
  if( held & ( WII_BUTTON_CV_4 | WII_CLASSIC_CV_4 ) || 
      gcHeld & GC_BUTTON_CV_4 )
  {
    //I|=wii_coleco_db_entry.button[3];
    I|=get_button_value( I, 3 );
  }

  // Button 5
  if( held & WII_CLASSIC_CV_5 || gcHeld & GC_BUTTON_CV_5 )
  {
    //I|=wii_coleco_db_entry.button[4];
    I|=get_button_value( I, 4 );
  }

  // Button 6
  if( held & WII_CLASSIC_CV_6 || gcHeld & GC_BUTTON_CV_6 )
  {
    //I|=wii_coleco_db_entry.button[5];
    I|=get_button_value( I, 5 );
  }

  // Button 7
  if( held & WII_CLASSIC_CV_7 )
  {
    //I|=wii_coleco_db_entry.button[6];
    I|=get_button_value( I, 6 );
  }

  // Button 8
  if( held & WII_CLASSIC_CV_8 )
  {
    //I|=wii_coleco_db_entry.button[7];
    I|=get_button_value( I, 7 );
  }

  // Only if in Keypad as D-PAD via Analog mode and
  // a keypad button has not been pressed
  if( kPadAsDpad && !( I & JST_KEYPAD ) )
  { 
    if( wii_analog_left( expRjsX, gcRjsX ) )
    {
      I|=wii_analog_up( expRjsY, gcRjsY ) ?
          JST_1 :
            wii_analog_down( expRjsY, gcRjsY ) ?
              JST_7 : JST_4;
    }
    else if( wii_analog_right( expRjsX, gcRjsX ) )
    {
      I|=wii_analog_up( expRjsY, gcRjsY ) ?
          JST_3 :
            wii_analog_down( expRjsY, gcRjsY ) ?
              JST_9 : JST_6;
    }
    else if( wii_analog_up( expRjsY, gcRjsY ) )
    {
        I|=JST_2;
    }
    else if( wii_analog_down( expRjsY, gcRjsY ) )
    {
        I|= JST_8;
    }
  }

#if 0
  // Display the menu
  if( ( down & WII_BUTTON_HOME ) || ( gcDown & GC_BUTTON_HOME ) || wii_hw_button )
  {
    wii_menu_show();
  }
#endif

  return I;
}

#if 0
static u8 coleco_header[2]; 

void wii_coleco_patch_rom()
{
  coleco_header[0] = ROM_CARTRIDGE[0];
  coleco_header[1] = ROM_CARTRIDGE[1];

  ROM_CARTRIDGE[0] = 0x55;
  ROM_CARTRIDGE[1] = 0xAA;
}

void wii_coleco_unpatch_rom()
{
  ROM_CARTRIDGE[0] = coleco_header[0];
  ROM_CARTRIDGE[1] = coleco_header[1];
}
#endif