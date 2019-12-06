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

#include <SDL.h>
#include <SDL_image.h>

#include <gctypes.h>

#include "Coleco.h"

#include "keypad_small_png.h"
#include "keypad_med_png.h"
#include "keypad_large_png.h"

#include "wii_app.h"
#include "wii_gx.h"
#include "wii_input.h"
#include "wii_sdl.h"

#include "wii_coleco.h"

extern char debug_str[255];

// Whether we tried to load the current cartridge's keypad data
static BOOL cart_keypad_load = FALSE;

// Whether to dim the screen
static BOOL dim_screen = FALSE;

#define PAD_X   15
#define PAD_Y   70

#define KEY_1             0x00
#define KEY_2             0x01
#define KEY_3             0x02
#define KEY_4             0x03
#define KEY_5             0x04
#define KEY_6             0x05
#define KEY_7             0x06
#define KEY_8             0x07
#define KEY_9             0x08
#define KEY_ASTERICK      0x09
#define KEY_0             0x0a
#define KEY_POUND         0x0b

#define JOY_LEFT          0x00
#define JOY_RIGHT         0x01
#define JOY_UP            0x02
#define JOY_DOWN          0x03
#define JOY_SELECT        0x04
#define JOY_TOGGLE_PAD    0x05
#define JOY_NONE          0xff

// The large keypad image
static gx_imagedata* large_keypad_idata = NULL;

// The medium keypad image
static gx_imagedata* med_keypad_idata = NULL;

// The small keypad image
static gx_imagedata* small_keypad_idata = NULL;

// The current cartridge's keypad image
static gx_imagedata* cart_keypad_idata = NULL;

// Large keypad
static ColecoOverlay large_keypad = 
  { 
    "",
    { 0x00, 0xFF, 0x00, 0x60 },
    21, 20, 27, 126, 6, 5
  };

// Medium keypad
static ColecoOverlay med_keypad = 
  { 
    "",
    { 0x00, 0xFF, 0x00, 0x60 },
    18, 16, 25, 104, 4, 5
  };

// Small keypad
static ColecoOverlay small_keypad = 
  { 
    "",
    { 0x00, 0xFF, 0x00, 0x60 },
    12, 11, 23, 80, 4, 4
  };

// Info about a keypad
typedef struct keypad {
  byte selected_key;    // The key that is currently selected
  BOOL visible;         // Whether the keypad is visible
  byte joy;             // The last joystick input
} keypad;

// Player 1's keypad
static keypad keypad0 = { KEY_1, FALSE, JOY_NONE };
// Player 2's keypad
static keypad keypad1 = { KEY_1, FALSE, JOY_NONE };

/*
 * Resets the keypads 
 */
void wii_keypad_reset()
{
  keypad0.visible = FALSE;
  keypad0.selected_key = KEY_1;
  keypad0.joy = JOY_NONE;
  keypad1.visible = FALSE;
  keypad1.selected_key = KEY_1;
  keypad1.joy = JOY_NONE;

  cart_keypad_load = FALSE;
  if( cart_keypad_idata != NULL )
  {
    wii_gx_freeimage( cart_keypad_idata );
    cart_keypad_idata = NULL;
  }  

  dim_screen = FALSE;
}

/*
 * Hides the specified keypad
 *
 * keypadIndex    The keypad to hide
 */
void wii_keypad_hide( int keypadIndex )
{
  keypad* kp = ((keypadIndex == 0) ? &keypad0 : &keypad1 );
  if( kp->visible )
  {
    kp->visible = FALSE;
    kp->joy = JOY_NONE;
  }
}

/*
 * Hides both of the keypads
 */
void wii_keypad_hide_both()
{
  wii_keypad_hide( 0 );
  wii_keypad_hide( 1 );
}

/*
 * Whether to dim the screen when the keypads are displayed
 *
 * dim  Whether to dim the screen when the keypads are displayed
 */
void wii_keypad_set_dim_screen( BOOL dim )
{
  dim_screen = dim;
}

/*
 * Returns the value of the selected button for the pad
 *
 * pad      The pad
 * return   the value of the selected button for the pad
 */
static u32 get_selected_button_value( keypad *pad )
{
  switch( pad->selected_key )
  {
    case KEY_1: return JST_1;
    case KEY_2: return JST_2;
    case KEY_3: return JST_3;
    case KEY_4: return JST_4;
    case KEY_5: return JST_5;
    case KEY_6: return JST_6;
    case KEY_7: return JST_7;
    case KEY_8: return JST_8;
    case KEY_9: return JST_9;
    case KEY_0: return JST_0;
    case KEY_ASTERICK: return JST_STAR;
    case KEY_POUND: return JST_POUND;
  }

  return JST_1;
}

/*
 * Renders the description for the specified keypad
 *
 * pad    The pad to render the description for
 * clear  Are we clearing the description?
 */
static void render_description( keypad *pad )
{
  char* desc = 
    wii_coleco_db_get_button_description(  
      get_selected_button_value( pad ), &wii_coleco_db_entry );

  if( strlen( desc ) == 0 )
  {
    return;
  }

  int padding = 5;  // padding around text
  int x = PAD_X + padding; // x location
  int y = PAD_Y - 5;       // y location  
  
  int pixelSize = 18;
  int w = wii_gx_gettextwidth( pixelSize, desc ); // Text width
  int h = pixelSize;   // Text height

  if( pad == &keypad1 )
  {
    x = WII_WIDTH_DIV2 - x - w; 
  }
  else
  {
    x = -WII_WIDTH_DIV2 + x;
  }

  y = WII_HEIGHT_DIV2 - y; 

  wii_gx_drawrectangle( 
    x + -padding, 
    y + h + padding, 
    w + (padding<<1), 
    h + (padding<<1), 
    (GXColor){0x0, 0x0, 0x0, 0x80}, TRUE );

  wii_gx_drawtext( x, y, pixelSize, desc, ftgxWhite, FTGX_ALIGN_BOTTOM );
}

/*
 * Renders the keypad
 *
 * pad  The keypad to render
 */
static void render_pad( keypad *pad )
{
  if( pad->visible )
  {
    // TODO: Cache these values...
    int keyw, keyh, offsetx, offsety, gapx, gapy;
    GXColor color;

    gx_imagedata* idata = NULL;
    
    // Use overlays if enabled
    if( ( wii_coleco_db_entry.overlayMode == OVERLAY_ENABLED ) ||
        ( ( wii_coleco_db_entry.overlayMode == OVERLAY_DEFAULT ) &&
          wii_use_overlay ) ) 
    {
      idata = cart_keypad_idata;
    }

    ColecoOverlay *overlay = NULL;
    if( idata == NULL )
    {
      int keypadSize = 
        ( ( wii_coleco_db_entry.keypadSize == KEYPAD_SIZE_DEFAULT ) ?
              wii_keypad_size : wii_coleco_db_entry.keypadSize );

      switch( keypadSize )
      {
        case KEYPAD_SIZE_LARGE:
          idata = large_keypad_idata;
          overlay = &large_keypad;
          break;
        case KEYPAD_SIZE_MED:
          idata = med_keypad_idata;
          overlay = &med_keypad;
          break;
        case KEYPAD_SIZE_SMALL:
          idata = small_keypad_idata;
          overlay = &small_keypad;
          break;
      }
    }
    else
    {      
      overlay = &(wii_coleco_db_entry.overlay);
    }

    keyw    = overlay->keypSelKeyW;
    keyh    = overlay->keypSelKeyH;
    offsetx = overlay->keypOffX;
    offsety = overlay->keypOffY;
    gapx    = overlay->keypGapX;
    gapy    = overlay->keypGapY;
    color   = (GXColor)
      { 
        overlay->keypSelKeyRgba.R,
        overlay->keypSelKeyRgba.G,
        overlay->keypSelKeyRgba.B,
        overlay->keypSelKeyRgba.A
      };

    if( idata != NULL )
    {
      int padx = 
        pad == &keypad0 ?
          PAD_X + -WII_WIDTH_DIV2 : 
          WII_WIDTH_DIV2 - PAD_X - idata->width;
      int pady = WII_HEIGHT_DIV2 - PAD_Y;     

      wii_gx_drawimage( padx, pady, 
        idata->width, idata->height, idata->data, 
        0, 1.0, 1.0, 0xFF );

      wii_gx_drawrectangle( 
        padx + offsetx + 
         ( ( pad->selected_key % 3 ) * ( keyw + gapx ) ), 
        pady - offsety - 
         ( ( pad->selected_key / 3 ) * ( keyh + gapy ) ), 
        keyw, keyh, color, TRUE );    
    }

    // Render the description
    render_description( pad );
  }
}

/*
 * Updates the state of the specified keypad
 *
 * pad      The keypad to update
 * joy      The current joystick input
 * return   State indicating keypad selection  
 */
static unsigned int update_keypad( keypad *pad, byte joy )
{
  if( pad->joy != JOY_NONE && joy != JOY_SELECT  )
  {
    // Ignore subsequent joystick events (simulate button down event)
    return 0;
  }

  switch( joy )
  {
    case JOY_TOGGLE_PAD:
      pad->visible = !pad->visible;
      break;
    case JOY_SELECT:
      {
        return get_selected_button_value( pad );
      }
      break;
    case JOY_RIGHT:
      if( pad->selected_key < KEY_POUND )
      {
        pad->selected_key++;
      }
      break;
    case JOY_LEFT:
      if( pad->selected_key > KEY_1 )
      {
        pad->selected_key--;
      }
      break;
    case JOY_DOWN:
      if( (int)pad->selected_key + 3 <= KEY_POUND )
      {
        pad->selected_key+=3;
      }
      break;
    case JOY_UP:
      if( (int)pad->selected_key - 3 >= KEY_1 )
      {
        pad->selected_key-=3;
      }
      break;
  }

  pad->joy = joy;
  return 0;
}

/*
 * Poll the specified joystick for events (0-based)
 *
 * joyIndex   The joystick to poll (0-based)
 * pad        The keypad associated with the specified joystick
 * return     State indicating keypad selection   
 */
static unsigned int poll_joystick( int joyIndex, keypad *pad )
{
  unsigned int ret = 0;

  // Check the state of the controllers
  u32 held = WPAD_ButtonsHeld( joyIndex );
  u32 gcHeld = PAD_ButtonsHeld( joyIndex );

  expansion_t exp;
  WPAD_Expansion( joyIndex, &exp );
  bool isClassic = ( exp.type == WPAD_EXP_CLASSIC );

  float expX = wii_exp_analog_val( &exp, TRUE, FALSE );
  float expY = wii_exp_analog_val( &exp, FALSE, FALSE );
  s8 gcX = PAD_StickX( joyIndex );
  s8 gcY = PAD_StickY( joyIndex );

  float expRjsX = wii_exp_analog_val( &exp, TRUE, TRUE );
  float expRjsY = wii_exp_analog_val( &exp, FALSE, TRUE );
  s8 gcRjsX = PAD_SubStickX( joyIndex );
  s8 gcRjsY = PAD_SubStickY( joyIndex );

  if( pad->visible )
  {
    // Whether the Wiimote is horizontal
    u8 wmHorizontal = wii_coleco_db_entry.wiiMoteHorizontal;

    // Right
    if( wii_digital_right( wmHorizontal, 1, held ) || 
        ( gcHeld & GC_BUTTON_CV_RIGHT ) ||
        wii_analog_right( expX, gcX ) || 
        wii_analog_right( expRjsX, gcRjsX ) )
    {
      update_keypad( pad, JOY_RIGHT );
    }
    else if( pad->joy == JOY_RIGHT )
    {
      // Simulate button-up event
      pad->joy = JOY_NONE;
    }

    // Left
    if( wii_digital_left( wmHorizontal, 0, held ) || 
        ( isClassic ? ( held & WII_CLASSIC_CV_LEFT ) : 0 ) ||
        ( gcHeld & GC_BUTTON_CV_LEFT ) || 
        wii_analog_left( expX, gcX ) ||
        wii_analog_left( expRjsX, gcRjsX ) )
    {
      update_keypad( pad, JOY_LEFT );
    }
    else if( pad->joy == JOY_LEFT )
    {
      // Simulate button-up event
      pad->joy = JOY_NONE;
    }

    // Down
    if( wii_digital_down( wmHorizontal, 1, held ) || 
        ( gcHeld & GC_BUTTON_CV_DOWN ) || 
        wii_analog_down( expY, gcY ) || 
        wii_analog_down( expRjsY, gcRjsY ) )
    {
      update_keypad( pad, JOY_DOWN );
    }
    else if( pad->joy == JOY_DOWN )
    {
      // Simulate button-up event
      pad->joy = JOY_NONE;
    }

    // Up
    if( wii_digital_up( wmHorizontal, 0, held ) ||
      ( isClassic ? ( held & WII_CLASSIC_CV_UP ) : 0 ) ||
      ( gcHeld & GC_BUTTON_CV_UP ) || 
      wii_analog_up( expY, gcY ) ||
      wii_analog_up( expRjsY, gcRjsY ) )
    {
      update_keypad( pad, JOY_UP );
    }
    else if( pad->joy == JOY_UP )
    {
      // Simulate button-up event
      pad->joy = JOY_NONE;
    }

    // Select
    if( ( held & 
          ( WII_BUTTON_CV_1 | WII_BUTTON_CV_2 |
            WII_BUTTON_CV_3 | WII_BUTTON_CV_4 |
            ( isClassic ? 
              ( WII_CLASSIC_CV_1 | WII_CLASSIC_CV_2 ) : 
              ( WII_NUNCHECK_CV_1 | WII_NUNCHECK_CV_2 ) ) ) ) || 
        ( gcHeld & ( GC_BUTTON_CV_1 | GC_BUTTON_CV_2 ) ) )
    {      
      ret = update_keypad( pad, JOY_SELECT );
    }
    else if( pad->joy == JOY_SELECT )
    {
      pad->joy = JOY_NONE;
    }
  }

  // Toggle the keypad display
  if( held & WII_BUTTON_CV_SHOW_KEYPAD || gcHeld & GC_BUTTON_CV_SHOW_KEYPAD )
  {
    update_keypad( pad, JOY_TOGGLE_PAD );
  }
  else if( pad->joy == JOY_TOGGLE_PAD )
  {
    // Simulate button-up event
    pad->joy = JOY_NONE;
  }

  return ret;
}

/*
 * Check whether the specified keypad is visible (0-based)
 *
 * keypadIndex  The keypad to check
 * return       Whether the specified keypad is visible
 */
BOOL wii_keypad_is_visible( int keypadIndex )
{
  return
    keypadIndex == 0 ?
      keypad0.visible : keypad1.visible;
}

/*
 * Returns whether the result of keypad one is swapped 
 *
 * return   Whether the result of keypad one is swapped
 */
static BOOL is_swapped()
{
  return ( 
    wii_coleco_db_entry.controlsMode == CONTROLS_MODE_ROLLER ||
    wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING ||
    wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING_TILT );
}

/*
 * Returns whether the value specified contains a key press for the given
 * keypad.
 *
 * return   Whether the value specified contains a key press for the
 *          given keypad.
 */ 
BOOL wii_keypad_is_pressed( int keypadIndex, u32 value )
{
  // x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0
  int shift = ( is_swapped() || ( keypadIndex == 1 ) ) ? 16 : 0;
  return value&(0x0f<<shift);
}

/*
 * Polls the joysticks and returns state information regarding keypad 
 * selection.
 *
 * return   State indicating keypad selection
 */
u32 wii_keypad_poll_joysticks()
{
  if( is_swapped() )
  {
    return 
      poll_joystick( 0, &keypad0 )<<16;
  }
  else
  {
    return 
      poll_joystick( 0, &keypad0 ) | poll_joystick( 1, &keypad1 )<<16;
  }
}

/*
 * Renders the keypads
 */
void wii_keypad_render()
{
  if( small_keypad_idata == NULL )
  {
    small_keypad_idata = 
      wii_gx_loadimagefrombuff( keypad_small_png );
  }

  if( med_keypad_idata == NULL )
  {
    med_keypad_idata = 
      wii_gx_loadimagefrombuff( keypad_med_png );
  }

  if( large_keypad_idata == NULL )
  {
     large_keypad_idata = 
       wii_gx_loadimagefrombuff( keypad_large_png );
  }

  if( !cart_keypad_load )
  {
    cart_keypad_load = TRUE;

    if( wii_coleco_db_entry.overlay.keypOverlay[0] != '\0' )
    {
      char overlay[WII_MAX_PATH];
      snprintf( overlay, WII_MAX_PATH, "%s%s", wii_get_overlays_dir(),
        wii_coleco_db_entry.overlay.keypOverlay );
      cart_keypad_idata = wii_gx_loadimage( overlay );
    }
  }

  if( dim_screen )
  {
    wii_gx_drawrectangle( 
      -WII_WIDTH_DIV2, WII_HEIGHT_DIV2, 
      WII_WIDTH, WII_HEIGHT, 
      (GXColor){0x0, 0x0, 0x0, 0x80}, TRUE );
  }

  render_pad( &keypad0 );
  render_pad( &keypad1 );
}

/*
 * Frees the keypad resources
 */
void wii_keypad_free_resources()
{
  if( small_keypad_idata != NULL )
  {
    wii_gx_freeimage( small_keypad_idata );
    small_keypad_idata = NULL;
  }

  if( med_keypad_idata != NULL )
  {
    wii_gx_freeimage( med_keypad_idata );
    med_keypad_idata = NULL;
  }

  if( large_keypad_idata != NULL )
  {
    wii_gx_freeimage( large_keypad_idata );
    large_keypad_idata = NULL;
  }
  cart_keypad_load = FALSE;
  if( cart_keypad_idata != NULL )
  {
    wii_gx_freeimage( cart_keypad_idata );
    cart_keypad_idata = NULL;    
  }
}
