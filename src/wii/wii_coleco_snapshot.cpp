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
#include <sys/stat.h>
#include <time.h>

#include "Coleco.h"

#include "wii_app.h"
#include "wii_snapshot.h"
#include "wii_util.h"

#include "wii_coleco.h"
#include "wii_coleco_emulation.h"
#include "wii_coleco_snapshot.h"

#define MAX_SNAPSHOTS   10

/** The current snapshot index */
static int ss_index = 0;
/** Whether the snapshot associated with the current index exists */
static int ss_exists = -1;
/** The index of the latest snapshot */
static int ss_latest = -1; 

/**
 * Returns the name of the snapshot associated with the specified romfile and
 * the snapshot index 
 *
 * @param   romfile The rom file
 * @param   index The snapshot index
 * @param   buffer The output buffer to receive the name of the snapshot file
 *              (length must be WII_MAX_PATH)
 */
static void get_snapshot_name(
    const char *romfile, int index, char *buffer) {  
    char filename[WII_MAX_PATH];            
    Util_splitpath(romfile, NULL, filename);
    snprintf(buffer, WII_MAX_PATH, "%s%s.%d.%s",  
        wii_get_saves_dir(), filename, index, WII_SAVE_GAME_EXT);
}

/**
 * Clears information regarding the current snapshot
 * (whether it exists, etc.)
 */
static void clear_current_snapshot_info() {
    ss_exists = -1;
    ss_latest = -1;  
}

/**
 * Returns whether the current snapshot exists
 *
 * @return  Whether the current snapshot exists
 */
BOOL wii_snapshot_current_exists() {
    if (ss_exists == -1) {
        if (!wii_last_rom) {
            ss_exists = FALSE;
        } else {
            char savename[WII_MAX_PATH];
            wii_snapshot_handle_get_name(wii_last_rom, savename);
            ss_exists = Util_fileexists(savename);
        }
    }
    return ss_exists;
}

/**
 * Determines the index of the latest snapshot
 *
 * @return  The index of the latest snapshot
 */
static int get_latest_snapshot() {
    
    if (ss_latest == -1 && wii_last_rom) {
        time_t max = 0;    
        char savename[WII_MAX_PATH];
        for (int i = 0; i < MAX_SNAPSHOTS; i++) {
            get_snapshot_name(wii_last_rom, i, savename);
            
            struct stat st;
            if (stat(savename, &st) == 0) {
                if (st.st_mtime > max) {
                    max = st.st_mtime;
                    ss_latest = i;
                }            
            }                    
        }            
    }
    return ss_latest;    
}

/**
 * Returns the name of the snapshot associated with the specified romfile and
 * the current snapshot index 
 *
 * @param   romfile The rom file
 * @param   buffer The output buffer to receive the name of the snapshot file
 *              (length must be WII_MAX_PATH)
 */
extern "C" void wii_snapshot_handle_get_name(
    const char *romfile, char *buffer) {  
    get_snapshot_name(romfile, ss_index, buffer);
}

/**
 * Attempts to save the snapshot to the specified file name 
 * 
 * @param   The name to save the snapshot to
 * @return  Whether the snapshot was successful 
 */
extern "C" BOOL wii_snapshot_handle_save(char* filename) {
    clear_current_snapshot_info(); // force recheck
    return SaveSTA(filename);
}

/**
 * Resets snapshot related informatino. This method is typically invoked when
 * a new rom file is loaded.
 */
void wii_snapshot_reset() {
  ss_index = 0;
  clear_current_snapshot_info();
}


/**
 * Returns the index of the current snapshot. 
 *
 * @param   isLatest (out) Whether the current snapshot index is the latest
 *              snapshot (most recent)
 * @return  The index of the current snapshot
 */
int wii_snapshot_current_index(BOOL* isLatest) {
    *isLatest = (ss_index == get_latest_snapshot());
    return ss_index;
}

/** 
 * Moves to the next snapshot (next index)
 *
 * @return  The index that was moved to
 */
int wii_snapshot_next() {
    if (++ss_index == MAX_SNAPSHOTS) {
        ss_index = 0;
    }    
    clear_current_snapshot_info(); // force recheck
    
    return ss_index;
}

/**
 * Starts emulation with the current snapshot
 *
 * @return  Whether emulation was successfully started
 */ 
BOOL wii_start_snapshot() {
    if (!wii_last_rom) {
        return FALSE;
    }
    clear_current_snapshot_info(); // force recheck

    char savename[WII_MAX_PATH];
    wii_snapshot_handle_get_name(wii_last_rom, savename);  
    return wii_start_emulation(wii_last_rom, savename, FALSE, FALSE);
}
