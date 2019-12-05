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
//  The license for Colem as indicated by Marat Fayzullin, the author of     //
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

#include <stdio.h>

#include "Coleco.h"

#include "wii_app.h"
#include "wii_snapshot.h"
#include "wii_util.h"

#include "wii_coleco.h"
#include "wii_coleco_emulation.h"
#include "wii_coleco_snapshot.h"

/*
* Saves with the specified save name
*
* filename   The name of the save file
* return     Whether the save was successful
*/
BOOL wii_snapshot_handle_save( char* filename )
{
  return SaveSTA( filename );
}

/*
* Determines the save name for the specified rom file
*
* romfile  The name of the rom file
* buffer   The buffer to write the save name to
*/
void wii_snapshot_handle_get_name( const char *romfile, char *buffer )
{
  char filename[WII_MAX_PATH];            
  Util_splitpath( romfile, NULL, filename );
  snprintf( buffer, WII_MAX_PATH, "%s%s.%s",  
    wii_get_saves_dir(), filename, WII_SAVE_GAME_EXT );
}

/*
* Starts the emulator for the specified snapshot file.
*
* savefile The name of the save file to load. 
*/
BOOL wii_start_snapshot( char *savefile )
{
  BOOL succeeded = FALSE;
  BOOL seterror = FALSE;

  // Determine the extension
  char ext[WII_MAX_PATH];
  Util_getextension( savefile, ext );

  if( !strcmp( ext, WII_SAVE_GAME_EXT ) )
  {
    char savename[WII_MAX_PATH];

    // Get the save name (without extension)
    Util_splitpath( savefile, NULL, savename );

    int namelen = strlen( savename );
    int extlen = strlen( WII_SAVE_GAME_EXT );

    if( namelen > extlen )
    {
      // build the associated rom name
      savename[namelen - extlen - 1] = '\0';

      char romfile[WII_MAX_PATH];
      snprintf( 
        romfile, WII_MAX_PATH, "%s%s", wii_get_roms_dir(), savename );

      int exists = Util_fileexists( romfile );

      // Ensure the rom exists
      if( !exists )            
      {
        wii_set_status_message(
          "Unable to find associated ROM file." );                
        seterror = TRUE;
      }
      else
      {
        // launch the emulator for the save
        succeeded = wii_start_emulation( romfile, savefile, FALSE, FALSE );
      }
    }
  }

  if( !succeeded && !seterror )
  {
    wii_set_status_message( 
      "The file selected is not a valid saved state file." );    
  }

  return succeeded;
}
