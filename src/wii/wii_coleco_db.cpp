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

#include "Coleco.h"

#include "wii_app.h"
#include "wii_util.h"
#include "wii_coleco.h"
#include "wii_coleco_db.h"

#ifdef WII_NETTRACE
#include <network.h>
#include "net_print.h"  
#endif

#define DB_FILE_PATH WII_FILES_DIR "wiicolem.db"
#define DB_TMP_FILE_PATH WII_FILES_DIR "wiicolem.db.tmp"
#define DB_OLD_FILE_PATH WII_FILES_DIR "wiicolem.db.old"

/** The database file */
static char db_file[WII_MAX_PATH] = "";
/** The database temp file */
static char db_tmp_file[WII_MAX_PATH] = "";
/** The database old file */
static char db_old_file[WII_MAX_PATH] = "";

/**
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
const char* WiiButtonDescriptions[MAPPED_BUTTON_COUNT][CONTROLLER_NAME_COUNT] = { 
//    (None)       Wiimote   Nunchuk   Classic        GameCube
    { "Button 1",  "2",      "C",      "A",           "A" },
    { "Button 2",  "1",      "Z",      "B",           "B" },
    { "Button 3",  "A",      "(n/a)",  "X",           "X" },
    { "Button 4",  "B",      "(n/a)",  "Y",           "Y" },
    { "Button 5",  "(n/a)",  "(n/a)",  "R Trigger",   "R Trigger" },
    { "Button 6",  "(n/a)",  "(n/a)",  "L Trigger",   "L Trigger" },
    { "Button 7",  "(n/a)",  "(n/a)",  "ZR Trigger",  "(n/a)" },
    { "Button 8",  "(n/a)",  "(n/a)",  "ZL Trigger",  "(n/a)" }                                                            
};

/**
 * The names of the Wii controllers 
 */
const char* WiiControllerNames[CONTROLLER_NAME_COUNT] = {
    "(None)", "Wiimote", "Nunchuk",  "Classic", "GameCube"
};

/**
 * The Coleco button values and their associated names
 */
ColecoButtonName ButtonNames[COLECO_BUTTON_NAME_COUNT] = { 
    { JST_NONE,     "(none)"        },
    { JST_FIRER,    "Right fire"    }, // Red     Brake
    { JST_FIREL,    "Left fire"     }, // Yellow  Gas
    { JST_PURPLE,   "Purple"        },
    { JST_BLUE,     "Blue"          },
    { JST_2P_FIRER, "2P Right fire" },
    { JST_2P_FIREL, "2P Left fire"  },
    { JST_0,        "Keypad 0"      }, 
    { JST_1,        "Keypad 1"      },
    { JST_2,        "Keypad 2"      },
    { JST_3,        "Keypad 3"      },
    { JST_4,        "Keypad 4"      },
    { JST_5,        "Keypad 5"      },
    { JST_6,        "Keypad 6"      },
    { JST_7,        "Keypad 7"      },
    { JST_8,        "Keypad 8"      },
    { JST_9,        "Keypad 9"      },
    { JST_STAR,     "Keypad *"      },
    { JST_POUND,    "Keypad #"      }
};

/**
 * Returns the path to the database file
 *
 * @return  The path to the database file
 */
static char* get_db_path() {
    if (db_file[0] == '\0') {
        snprintf(db_file, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 DB_FILE_PATH);
    }
    return db_file;
}

/**
 * Returns the path to the database temporary file
 *
 * @return  The path to the database temporary file
 */
static char* get_db_tmp_path() {
    if (db_tmp_file[0] == '\0') {
        snprintf(db_tmp_file, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 DB_TMP_FILE_PATH);
    }
    return db_tmp_file;
}

/**
 * Returns the path to the database old file
 *
 * @return  The path to the database old file
 */
static char* get_db_old_path() {
    if (db_old_file[0] == '\0') {
        snprintf(db_old_file, WII_MAX_PATH, "%s%s", wii_get_fs_prefix(),
                 DB_OLD_FILE_PATH);
    }
    return db_old_file;
}

/**
 * Whether to pause the keypad if it is displayed
 *
 * @param   entry The database entry
 * @return  Whether to pause the keypad if it is displayed
 */
BOOL wii_keypad_is_pause_enabled(ColecoDBEntry* entry) {
    return (
        (entry->keypadPause == KEYPAD_PAUSE_ENABLED) ||
        ((entry->keypadPause == KEYPAD_PAUSE_DEFAULT) && (wii_keypad_pause)));
}

/**
 * Populates the specified entry with default values. The default values
 * are based on the "controlsMode" in the specified entry.
 *
 * @param   entry The entry to populate with default values
 * @param   fullClear Whether to fully clear the entry
 */
void wii_coleco_db_get_defaults(ColecoDBEntry* entry, BOOL fullClear) {
    // Whether the WiiMote is horizontal
    entry->wiiMoteHorizontal = 0;

    // Default button mappings
    memset(entry->button, JST_NONE, sizeof(entry->button));

    switch (entry->controlsMode) {
        case CONTROLS_MODE_STANDARD:
            entry->button[0] = JST_FIREL;
            entry->button[1] = JST_FIRER;
            entry->wiiMoteHorizontal = 1;
            break;
        case CONTROLS_MODE_SUPERACTION:
            entry->button[0] = entry->button[4] = JST_FIREL;  // Yellow
            entry->button[1] = entry->button[5] = JST_FIRER;  // Orange
            entry->button[2] = entry->button[6] = JST_BLUE;
            entry->button[3] = entry->button[7] = JST_PURPLE;
            break;
        case CONTROLS_MODE_ROLLER:
            entry->button[0] = JST_2P_FIREL;
            entry->button[1] = JST_2P_FIRER;
            entry->button[2] = JST_FIREL;
            entry->button[3] = JST_FIRER;
            break;
        case CONTROLS_MODE_DRIVING:
            entry->button[0] = JST_FIREL;
            entry->button[1] = JST_FIRER;
            break;
        case CONTROLS_MODE_DRIVING_TILT:
            entry->button[0] = JST_FIREL;
            entry->button[1] = JST_FIRER;
            entry->wiiMoteHorizontal = 1;
            break;
    }

    // Whether to fully clear the entry
    if (fullClear) {
        entry->keypadPause = KEYPAD_PAUSE_DEFAULT;
        entry->sensitivity = 5;
    }
}

/**
 * Attempts to locate a hash in the specified source string. If it
 * is found, it is copied into dest.
 *
 * @param   source The source string
 * @param   dest The destination string
 * @return  non-zero if the hash was found and copied
 */
static int get_hash(char* source, char* dest) {
    int db_hash_len = 0;  // Length of the hash
    char* end_idx;        // End index of the hash
    char* start_idx;      // Start index of the hash

    start_idx = source;
    if (*start_idx == '[') {
        ++start_idx;
        end_idx = strrchr(start_idx, ']');
        if (end_idx != 0) {
            db_hash_len = end_idx - start_idx;
            strncpy(dest, start_idx, db_hash_len);
            dest[db_hash_len] = '\0';
            return 1;
        }
    }

    return 0;
}

/**
 * Returns whether the button at the specified index is available for
 * the specified mode.
 *
 * @param   index The button index
 * @param   mode The controller mode
 * @return  Whether the button is available for the specified mode
 */
int wii_coleco_db_is_button_available(u16 index, u8 mode) {
    u32 buttonValue = ButtonNames[index].button;

    switch (mode) {
        case CONTROLS_MODE_DRIVING:
        case CONTROLS_MODE_DRIVING_TILT:
        case CONTROLS_MODE_STANDARD:
            return buttonValue != JST_PURPLE && buttonValue != JST_BLUE &&
                   buttonValue != JST_2P_FIRER && buttonValue != JST_2P_FIREL;
        case CONTROLS_MODE_ROLLER:
            return buttonValue != JST_PURPLE && buttonValue != JST_BLUE;
        case CONTROLS_MODE_SUPERACTION:
            return buttonValue != JST_2P_FIRER && buttonValue != JST_2P_FIREL;
    }
    return 1;
}

/**
 * Copies the name of the button with the specified value into the
 * buffer
 *
 * @param   buff The buffer to copy the name into
 * @param   buffsize The size of the buffer
 * @param   value The value of the button
 * @param   entry The database entry
 */
void wii_coleco_db_get_button_name(char* buff,
                                   int buffsize,
                                   u32 value,
                                   ColecoDBEntry* entry) {
    const char* btnName = 0;
    int i;
    for (i = 0; i < COLECO_BUTTON_NAME_COUNT; i++) {
        ColecoButtonName name = ButtonNames[i];
        if (value == name.button) {
            switch (entry->controlsMode) {
                case CONTROLS_MODE_SUPERACTION:
                    switch (value) {
                        case JST_FIRER:
                            btnName = "Orange";
                            break;
                        case JST_FIREL:
                            btnName = "Yellow";
                            break;
                    }
                    break;
                case CONTROLS_MODE_DRIVING:
                case CONTROLS_MODE_DRIVING_TILT:
                    switch (value) {
                        case JST_FIRER:
                            btnName = "Brake";
                            break;
                        case JST_FIREL:
                            btnName = "Gas";
                            break;
                    }
                    break;
            }

            if (btnName == 0) {
                btnName = name.name;
            }
        }
    }

    if (btnName == 0) {
        btnName = "(none)";
    }

    char* desc = wii_coleco_db_get_button_description(value, entry);
    if (desc[0] == '\0') {
        snprintf(buff, buffsize, "%s", btnName);
    } else {
        snprintf(buff, buffsize, "%s (%s)", desc, btnName);
    }
}

/**
 * Returns the index of the button in the list of mappable buttons
 *
 * @param   value The button value;
 * @return  The index of the button in the list of mappable buttons
 */
int wii_coleco_db_get_button_index(u32 value) {
    int i;
    for (i = 0; i < COLECO_BUTTON_NAME_COUNT; i++) {
        ColecoButtonName name = ButtonNames[i];
        if (value == name.button) {
            return i;
        }
    }
    return 0;
}

/**
 * Returns the description for the button with the specified value
 *
 * @param   value The button value
 * @param   entry The entry containing the button descriptions
 * @return  The description for the button with the specified value
 */
char* wii_coleco_db_get_button_description(u32 value, ColecoDBEntry* entry) {
    int index = wii_coleco_db_get_button_index(value);
    return entry->buttonDesc[index];
}

/**
 * Writes the database entry to the specified file
 *
 * @param   file The file to write the entry to
 * @param   hash The hash for the entry
 * @param   entry The entry
 */
static void write_entry(FILE* file, char* hash, ColecoDBEntry* entry) {
    int i;
    if (!entry)
        return;

    char hex[64] = "";
    Util_rgbatohex(&(entry->overlay.keypSelKeyRgba), hex);

    fprintf(file, "[%s]\n", hash);
    fprintf(file, "name=%s\n", entry->name);
    fprintf(file, "controlsMode=%d\n", entry->controlsMode);
    fprintf(file, "wmHorizontal=%d\n", entry->wiiMoteHorizontal);
    fprintf(file, "flags=%d\n", entry->flags);
    fprintf(file, "eeprom=%d\n", entry->eeprom);
    fprintf(file, "sensitivity=%d\n", entry->sensitivity);
    fprintf(file, "keypadPause=%d\n", entry->keypadPause);
    fprintf(file, "keypadSize=%d\n", entry->keypadSize);
    fprintf(file, "cycleAdjust=%d\n", entry->cycleAdjust);
    fprintf(file, "maxFrames=%d\n", entry->maxFrames);
    fprintf(file, "overlayMode=%d\n", entry->overlayMode);
    fprintf(file, "keypOverlay=%s\n", entry->overlay.keypOverlay);
    fprintf(file, "keypSelKeyRgba=%s\n", hex);
    fprintf(file, "keypSelKeyW=%d\n", entry->overlay.keypSelKeyW);
    fprintf(file, "keypSelKeyH=%d\n", entry->overlay.keypSelKeyH);
    fprintf(file, "keypOffX=%d\n", entry->overlay.keypOffX);
    fprintf(file, "keypOffY=%d\n", entry->overlay.keypOffY);
    fprintf(file, "keypGapX=%d\n", entry->overlay.keypGapX);
    fprintf(file, "keypGapY=%d\n", entry->overlay.keypGapY);

    for (i = 0; i < MAPPED_BUTTON_COUNT; i++) {
        fprintf(file, "button%d=%d\n", i, entry->button[i]);
    }
    for (i = 0; i < COLECO_BUTTON_NAME_COUNT; i++) {
        char* desc = entry->buttonDesc[i];
        if (desc[0] != '\0') {
            ColecoButtonName* name = &(ButtonNames[i]);
            fprintf(file, "buttonDesc%d=%s\n", name->button, desc);
        }
    }
}

/**
 * Returns the database entry for the game with the specified hash
 *
 * @param   hash The hash of the game
 * @param   entry The entry to populate for the specified game
 */
void wii_coleco_db_get_entry(char* hash, ColecoDBEntry* entry) {
    char buff[255];     // The buffer to use when reading the file
    FILE* db_file;      // The database file
    char db_hash[255];  // A hash found in the file we are reading from
    int read_mode = 0;  // The current read mode
    char* ptr;          // Pointer into the current entry value

    // Populate the entry with the defaults
    memset(entry, 0x0, sizeof(ColecoDBEntry));
    entry->controlsMode = CONTROLS_MODE_STANDARD;
    wii_coleco_db_get_defaults(entry, TRUE);

    db_file = fopen(get_db_path(), "r");

#ifdef WII_NETTRACE
    char val[256];
    sprintf(val, "fopen %s=%p\n", get_db_path(), db_file);
    net_print_string(__FILE__, __LINE__, val);
#endif

    if (db_file != 0) {
        while (fgets(buff, sizeof(buff), db_file) != 0) {
            if (read_mode == 2) {
                // We moved past the current record, exit.
                break;
            }
            if (read_mode == 1) {
                // Read from the matching database entry
                ptr = strchr(buff, '=');
                if (ptr) {
                    *ptr++ = '\0';
                    Util_trim(buff);
                    Util_trim(ptr);

                    if (!strcmp(buff, "name")) {
                        Util_strlcpy(entry->name, ptr, sizeof(entry->name));
                    } else if (!strcmp(buff, "controlsMode")) {
                        entry->controlsMode = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "wmHorizontal")) {
                        entry->wiiMoteHorizontal = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "flags")) {
                        entry->flags = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "eeprom")) {
                        entry->eeprom = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypadPause")) {
                        entry->keypadPause = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypadSize")) {
                        entry->keypadSize = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "sensitivity")) {
                        entry->sensitivity = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "cycleAdjust")) {
                        entry->cycleAdjust = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "maxFrames")) {
                        entry->maxFrames = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "overlayMode")) {
                        entry->overlayMode = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypSelKeyW")) {
                        entry->overlay.keypSelKeyW = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypSelKeyH")) {
                        entry->overlay.keypSelKeyH = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypOffX")) {
                        entry->overlay.keypOffX = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypOffY")) {
                        entry->overlay.keypOffY = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypGapX")) {
                        entry->overlay.keypGapX = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypGapY")) {
                        entry->overlay.keypGapY = Util_sscandec(ptr);
                    } else if (!strcmp(buff, "keypSelKeyRgba")) {
                        Util_hextorgba(ptr, &(entry->overlay.keypSelKeyRgba));
                    } else if (!strcmp(buff, "keypOverlay")) {
                        Util_strlcpy(entry->overlay.keypOverlay, ptr,
                                     sizeof(entry->overlay.keypOverlay));
                    }

                    int i;
                    char button[255];
                    for (i = 0; i < MAPPED_BUTTON_COUNT; i++) {
                        snprintf(button, sizeof(button), "button%d", i);
                        if (!strcmp(buff, button)) {
                            entry->button[i] = Util_sscandec(ptr);
                        }
                    }
                    for (i = 0; i < COLECO_BUTTON_NAME_COUNT; i++) {
                        snprintf(button, sizeof(button), "buttonDesc%d",
                                 ButtonNames[i].button);
                        if (!strcmp(buff, button)) {
                            Util_strlcpy(entry->buttonDesc[i], ptr,
                                         sizeof(entry->buttonDesc[i]));
                        }
                    }
                }
            }

            // Search for the hash
            if (get_hash(buff, db_hash) && read_mode < 2) {
                if (read_mode || !strcmp(hash, db_hash)) {
                    entry->loaded = 1;
                    read_mode++;
                }
            }
        }
        fclose(db_file);
    }
}

/**
 * Deletes the entry from the database with the specified hash
 *
 * @param   hash The hash of the game
 * @return  Whether the delete was successful
 */
int wii_coleco_db_delete_entry(char* hash) {
    return wii_coleco_db_write_entry(hash, 0);
}

/**
 * Writes the specified entry to the database for the game with the specified
 * hash.
 *
 * @param   hash The hash of the game
 * @param   entryThe entry to write to the database (null to delete the entry)
 * @return  Whether the write was successful
 */
int wii_coleco_db_write_entry(char* hash, ColecoDBEntry* entry) {
    char buff[255];      // The buffer to use when reading the file
    char db_hash[255];   // A hash found in the file we are reading from
    int copy_mode = 0;   // The current copy mode
    FILE* tmp_file = 0;  // The temp file
    FILE* old_file = 0;  // The old file

    // The database file
    FILE* db_file = fopen(get_db_path(), "r");

    // A database file doesn't exist, create a new one
    if (!db_file) {
        db_file = fopen(get_db_path(), "w");
        if (!db_file) {
            // Unable to create DB file
            return 0;
        }

        // Write the entry
        write_entry(db_file, hash, entry);

        fclose(db_file);
    } else {
        //
        // A database exists, search for the appropriate hash while copying
        // its current contents to a temp file
        //

        // Open up the temp file
        tmp_file = fopen(get_db_tmp_path(), "w");
        if (!tmp_file) {
            fclose(db_file);

            // Unable to create temp file
            return 0;
        }

        //
        // Loop and copy
        //

        while (fgets(buff, sizeof(buff), db_file) != 0) {
            // Check if we found a hash
            if (copy_mode < 2 && get_hash(buff, db_hash)) {
                if (copy_mode) {
                    copy_mode++;
                } else if (!strcmp(hash, db_hash)) {
                    // We have matching hashes, write out the new entry
                    write_entry(tmp_file, hash, entry);
                    copy_mode++;
                }
            }

            if (copy_mode != 1) {
                fprintf(tmp_file, "%s", buff);
            }
        }

        if (!copy_mode) {
            // We didn't find the hash in the database, add it
            write_entry(tmp_file, hash, entry);
        }

        fclose(db_file);
        fclose(tmp_file);

        //
        // Make sure the temporary file exists
        // We do this due to the instability of the Wii SD card
        //
        tmp_file = fopen(get_db_tmp_path(), "r");
        if (!tmp_file) {
            // Unable to find temp file
            return 0;
        }
        fclose(tmp_file);

        // Delete old file (if it exists)
        if ((old_file = fopen(get_db_old_path(), "r")) != 0) {
            fclose(old_file);
            if (remove(get_db_old_path()) != 0) {
                return 0;
            }
        }

        // Rename database file to old file
        if (rename(get_db_path(), get_db_old_path()) != 0) {
            return 0;
        }

        // Rename temp file to database file
        if (rename(get_db_tmp_path(), get_db_path()) != 0) {
            return 0;
        }
    }

    return 1;
}
