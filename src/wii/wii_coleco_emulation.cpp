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

#include <wiiuse/wpad.h>

#include "Coleco.h"
#include "Sound.h"

#include "wii_app_common.h"
#include "wii_app.h"
#include "wii_config.h"
#include "wii_file_io.h"
#include "wii_hash.h"
#include "wii_input.h"
#include "wii_sdl.h"
#include "wii_snapshot.h"

#include "wii_coleco.h"
#include "wii_coleco_keypad.h"
#include "wii_coleco_snapshot.h"

/** MasterSwitch extern from ColEm */
extern int MasterSwitch;

/** Reset cycle timing extern form ColEm */
extern void ResetCycleTiming(void);

/** SDL Video external references */
extern "C" {
void WII_VideoStop();
void WII_ChangeSquare(int xscale, int yscale, int xshift, int yshift);
void WII_SetRenderCallback(void (*cb)(void));
void WII_SetFilter(BOOL filter);
}

#if 1
extern u32 cur_xfb;
#endif

/**
 * Returns the mode flags for ColEm
 *
 * @return  The mode flags for ColEm
 */
static int get_coleco_mode() {
    return wii_coleco_mode |
           (wii_coleco_db_entry.flags & CART_SRAM ? CV_SRAM : 0) |
           (wii_coleco_db_entry.eeprom == EEPROM_24C256
                ? CV_24C256
                : (wii_coleco_db_entry.eeprom == EEPROM_24C08 ? CV_24C08 : 0)) |
           (wii_super_game_module ? CV_SGM : 0);
}

/**
 * Starts the emulator for the specified rom file.
 *
 * @param   romfile The rom file to run in the emulator
 * @param   savefile The name of the save file to load.
 *          If this value is NULL, no save is explicitly loaded.
 * @param   reset Whether we are resetting the current game
 * @param   resume Whether we are resuming the current game
 * @return  Whether the emulation started successfully
 */
BOOL wii_start_emulation(char* romfile,
                         const char* savefile,
                         BOOL reset,
                         BOOL resume) {
    // Write out the current config
    wii_write_config();

    // Whether emulation successfully started
    BOOL succeeded = true;

    // Start emulation
    if (!reset && !resume) {
        // Whether to load a save file
        BOOL loadsave = (savefile != NULL && strlen(savefile) > 0);

        // Determine the state file name (used for EEPROM carts)
        char state_file[WII_MAX_PATH];
        char filename[WII_MAX_PATH];
        Util_splitpath(romfile, NULL, filename);
        snprintf(state_file, WII_MAX_PATH, "%s%s.%s", wii_get_states_dir(),
                 filename, WII_STATE_GAME_EXT);

        // Clear the Coleco DB entry
        memset(&wii_coleco_db_entry, 0x0, sizeof(ColecoDBEntry));

        // If we are not loading a save, reset snapshot related state
        // information (current index, etc.)
        if (!loadsave) {
            wii_snapshot_reset();  // Reset snapshot related state
        }

        // Attempt to load the ROM
        succeeded = LoadROM(romfile, state_file);
        if (succeeded) {
            // Calculate the hash
            wii_hash_compute(ROM_CARTRIDGE, succeeded, wii_cartridge_hash);

            // Look up the cartridge in the database
            wii_coleco_db_get_entry(wii_cartridge_hash, &wii_coleco_db_entry);

            // Reset coleco.
            // Allows for memory adjustments based on cartridge settings, etc.
            ResetColeco(get_coleco_mode());
        } else {
            wii_set_status_message(
                "An error occurred loading the specified cartridge.");
        }

        // Attempt to load the save file if applicable
        if (loadsave) {
            // Ensure the save is valid
            int sscheck = wii_check_snapshot(savefile);
            if (sscheck < 0) {
                wii_set_status_message(
                    "Unable to find the specified save state file.");
                succeeded = false;
            } else {
                // Load the save file
                succeeded = LoadSTA(savefile);
                if (!succeeded) {
                    wii_set_status_message(
                        "An error occurred attempting to load the save state "
                        "file.");
                }
            }
        }
    } else if (reset) {
        // Reset the emulator
        ResetColeco(get_coleco_mode());
    } else if (resume) {
        // Restore mode prior to resume
        Mode = get_coleco_mode();
    }

    if (succeeded) {
        // Store the name of the last rom (for resuming later)
        // Do it in this order in case they passed in the pointer
        // to the last rom variable
        char* last = strdup(romfile);
        if (wii_last_rom != NULL) {
            free(wii_last_rom);
        }
        wii_last_rom = last;

        // Clear the screen
        wii_sdl_black_screen();
        VIDEO_WaitVSync();

        // Reset the keypad
        wii_keypad_reset();

        // Wait until no buttons are pressed
        wii_wait_until_no_buttons(2);

        // Update cycle period
        CPU.IPeriod = (Mode & CV_PAL ? TMS9929_LINE : TMS9918_LINE) +
                      (wii_coleco_db_entry.cycleAdjust) /*-23*/;

        // Reset cycle timing information
        ResetCycleTiming();

        // Set volume
        // 255/SN76489_CHANNELS = 63
        SetChannels((wii_volume == 0 ? 0 : (3 + (wii_volume * 4))),
                    MasterSwitch);

        // Set spinner controls
        Mode &= ~CV_SPINNERS;
        if ((wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING) ||
            (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_DRIVING_TILT) ||
            (wii_coleco_db_entry.controlsMode == CONTROLS_MODE_ROLLER) ||
            ((wii_coleco_db_entry.controlsMode == CONTROLS_MODE_SUPERACTION) &&
             !(wii_coleco_db_entry.flags & DISABLE_SPINNER))) {
            Mode |= CV_SPINNER1X;
            if ((wii_coleco_db_entry.controlsMode != CONTROLS_MODE_DRIVING) &&
                (wii_coleco_db_entry.controlsMode !=
                 CONTROLS_MODE_DRIVING_TILT)) {
                Mode |= CV_SPINNER2Y;
            }
        }
    } else {
        // Reset the last rom that was loaded
        if (wii_last_rom != NULL) {
            free(wii_last_rom);
            wii_last_rom = NULL;
        }
    }
    return succeeded;
}

/**
 * Resumes emulation of the current game
 *
 * @return  Whether we were able to resume emulation
 */
BOOL wii_resume_emulation() {
    return wii_start_emulation(wii_last_rom, "", FALSE, TRUE);
}

/**
 * Resets the current game
 *
 * @return  Whether we were able to reset emulation
 */
BOOL wii_reset_emulation() {
    return wii_start_emulation(wii_last_rom, "", TRUE, FALSE);
}
