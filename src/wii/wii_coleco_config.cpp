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

#include "wii_util.h"
#include "wii_coleco.h"

#include "networkop.h"

/**
 * Handles reading a particular configuration value
 *
 * @param   name The name of the config value
 * @param   value The config value
 */
extern "C" void wii_config_handle_read_value(char *name, char* value) {
    if (strcmp(name, "debug") == 0) {
        wii_debug = Util_sscandec(value);
    } else if (strcmp(name, "top_menu_exit") == 0) {
        wii_top_menu_exit = Util_sscandec(value);
    } else if (strcmp(name, "vsync") == 0) {
        wii_vsync = Util_sscandec(value);
    } else if (strcmp(name, "col_mode") == 0) {
        wii_coleco_mode = Util_sscandec( value );
    } else if (strcmp(name, "controller_view_mode") == 0) {
        wii_coleco_controller_view_mode = Util_sscandec(value);
    } else if (strcmp(name, "keypad_pause") == 0) {
        wii_keypad_pause = Util_sscandec(value);
    } else if (strcmp(name, "keypad_size") == 0) {
        wii_keypad_size = Util_sscandec(value);
    } else if (strcmp(name, "use_overlay") == 0) {
        wii_use_overlay = Util_sscandec(value);
    } else if (strcmp(name, "volume") == 0) {
        wii_volume = Util_sscandec(value);
    } else if (strcmp(name, "max_frames") == 0) {
        wii_max_frames = Util_sscandec(value);
    } else if (strcmp(name, "screen_size_x") == 0) {
        wii_screen_x = Util_sscandec(value);
    } else if (strcmp(name, "screen_size_y") == 0) {
        wii_screen_y = Util_sscandec(value);
    } else if (strcmp(name, "sel_offset") == 0) {
        wii_menu_sel_offset = Util_sscandec(value);
    } else if (strcmp(name, "sel_color") == 0) {
        Util_hextorgba(value, &wii_menu_sel_color);
    } else if (strcmp(name, "mote_menu_vertical") == 0) {
        wii_mote_menu_vertical = Util_sscandec(value);
    } else if (strcmp(name, "super_game_module") == 0) {
        wii_super_game_module = Util_sscandec(value);				
    } else if (strcmp(name, "16_9_correction") == 0) {
        wii_16_9_correction = Util_sscandec(value);
    } else if (strcmp(name, "full_widescreen") == 0) {
        wii_full_widescreen = Util_sscandec(value);
    } else if (strcmp(name, "video_filter") == 0) {
        wii_filter = Util_sscandec(value);
    } else if (strcmp(name, "vi_gx_scaler") == 0) {
        wii_gx_vi_scaler = Util_sscandec(value);
    } else if (strcmp(name, "roms_dir") == 0) {
        wii_set_roms_dir(value);
    } else if (strcmp(name, "share_ip") == 0) {
        setSmbAddress(value);
    } else if (strcmp(name, "share_name") == 0) {
        setSmbShare(value);
    } else if (strcmp(name, "share_user") == 0) {
        setSmbUser(value);
    } else if (strcmp(name, "share_pass") == 0) {
        setSmbPassword(value);
    } else if (strcmp(name, "usb_keepalive") == 0) {
        wii_usb_keepalive = Util_sscandec(value);
    }    
}

/**
 * Handles the writing of the configuration file
 *
 * @param   fp The pointer to the file to write to
 */
extern "C" void wii_config_handle_write_config(FILE *fp) {
    fprintf(fp, "debug=%d\n", wii_debug);
    fprintf(fp, "top_menu_exit=%d\n", wii_top_menu_exit);
    fprintf(fp, "vsync=%d\n", wii_vsync);
    fprintf(fp, "col_mode=%d\n", wii_coleco_mode);
    fprintf(fp, "controller_view_mode=%d\n", wii_coleco_controller_view_mode);
    fprintf(fp, "keypad_pause=%d\n", wii_keypad_pause);
    fprintf(fp, "keypad_size=%d\n", wii_keypad_size);
    fprintf(fp, "use_overlay=%d\n", wii_use_overlay);
    fprintf(fp, "volume=%d\n", wii_volume);
    fprintf(fp, "max_frames=%d\n", wii_max_frames);  
    fprintf(fp, "screen_size_x=%d\n", wii_screen_x);
    fprintf(fp, "screen_size_y=%d\n", wii_screen_y);
    fprintf(fp, "sel_offset=%d\n", wii_menu_sel_offset);  
    fprintf(fp, "mote_menu_vertical=%d\n", wii_mote_menu_vertical);  
    fprintf(fp, "super_game_module=%d\n", wii_super_game_module); 
    fprintf(fp, "16_9_correction=%d\n", wii_16_9_correction);  
    fprintf(fp, "full_widescreen=%d\n", wii_full_widescreen);   
    fprintf(fp, "video_filter=%d\n", wii_filter);  
    fprintf(fp, "vi_gx_scaler=%d\n", wii_gx_vi_scaler); 
    fprintf(fp, "roms_dir=%s\n", wii_get_roms_dir());     
    fprintf(fp, "share_ip=%s\n", getSmbAddress());
    fprintf(fp, "share_name=%s\n", getSmbShare());
    fprintf(fp, "share_user=%s\n", getSmbUser());    
    fprintf(fp, "share_pass=%s\n", getSmbPassword());   
    fprintf(fp, "usb_keepalive=%d\n", wii_usb_keepalive);   

    char hex[64] = "";
    Util_rgbatohex(&wii_menu_sel_color, hex);
    fprintf(fp, "sel_color=%s\n", hex);
}
