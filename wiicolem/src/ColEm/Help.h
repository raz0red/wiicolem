/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                          Help.h                         **/
/**                                                         **/
/** This file contains help information printed out by the  **/
/** main() routine when started with option "-help".        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

char *HelpText[] =
{
  "\nUsage: colem [-option1 [-option2...]] [filename]",
  "\n[filename] = Name of the file to load as a cartridge [CART.ROM]",
#if defined(ZLIB)
  "  This program will transparently uncompress singular GZIPped",
  "  and PKZIPped files.",
#endif
  "\n[-option]  =",
  "  -verbose <level>    - Select debugging messages [5]",
  "                        0 - Silent       1 - Startup messages",
  "                        2 - VDP          4 - Illegal Z80 ops",
  "                        8 - EEPROM      16 - Sound",
  "  -pal/-ntsc          - Video system to use [-ntsc]",
  "  -skip <percent>     - Percentage of frames to skip [25]",
  "  -help               - Print this help page",
  "  -home <dirname>     - Set directory with system ROM files [off]",
  "  -adam/-cv           - Run in Adam/ColecoVision mode [-cv]",
  "  -sgm/-nosgm         - Enable Super Game Module extension [-nosgm]",
  "  -24c08/-24c256      - Enable serial EEPROM emulation [off]",
  "  -sram               - Enable battery-backed SRAM emulation [off]",
  "  -allspr             - Show all sprites [off]",
  "  -autoa/-noautoa     - Autofire/No autofire for FIRE-R button [-noautoa]",
  "  -autob/-noautob     - Autofire/No autofire for FIRE-L button [-noautob]",

  "  -spin1x/-spin1y     - Mouse X/Y position as SuperAction spinner 1 [off]",
  "  -spin2x/-spin2y     - Mouse X/Y position as SuperAction spinner 2 [off]",
  "  -drums/-nodrums     - Hit/Don't hit MIDI drums on noise [-nodrums]",
  "  -logsnd <filename>  - Write soundtrack to a MIDI file [LOG.MID]",
  "  -palette <number>   - Use given color palette [0]",
  "                        0 - Scaled VDP colors   1 - Original VDP colors",
  "                        2 - Faded NTSC colors",
  "  -sound [<quality>]  - Sound emulation quality [22050]",
  "  -nosound            - Don't emulate sound [-nosound]",

#if defined(DEBUG)
  "  -trap <address>     - Trap execution when PC reaches address [FFFFh]",
#endif /* DEBUG */

#if defined(MSDOS) || defined(UNIX) || defined(MAEMO)
  "  -sync <frequency>   - Sync screen updates to <frequency> [-sync 60]",
  "  -nosync             - Do not sync screen updates",
  "  -tv/-lcd/-raster    - Simulate TV scanlines or LCD raster [off]",
  "  -soft/-eagle        - Scale display with 2xSaI or EAGLE [off]",
  "  -epx/-scale2x       - Scale display with EPX or Scale2X [off]",
  "  -cmy/-rgb           - Simulate CMY/RGB pixel raster [off]",
  "  -mono/-sepia        - Simulate monochrome or sepia CRT [off]",
  "  -green/-amber       - Simulate green or amber CRT [off]",
  "  -4x3                - Force 4:3 television screen ratio [off]",
#endif /* MSDOS || UNIX || MAEMO */

#if defined(UNIX) || defined(MAEMO)
  "  -saver/-nosaver     - Save/don't save CPU when inactive [-saver]",
#endif /* UNIX || MAEMO */

#if defined(UNIX)
#if defined(MITSHM)
  "  -shm/-noshm         - Use/don't use MIT SHM extensions for X [-shm]",
#endif
  "  -scale <factor>     - Scale window by <factor> [2]",
#endif /* UNIX */

#if defined(MSDOS)
  "  -vsync              - Sync screen updates to VGA VBlanks [-vsync]",
#endif /* MSDOS */

  "\nKeyboard bindings:",
  "  [ALT]          - Hold to switch to the second controller",
  "  [SPACE]        - FIRE-R button (also: [SHIFT],A,S,D,F,G,H,J,K,L)",
  "  [CONTROL]      - FIRE-L button (also: Z,X,C,V,B,N,M)",
  "  [Q]            - SuperAction PURPLE button (also: E,T,U,O)",
  "  [W]            - SuperAction BLUE button (also: R,Y,I,P)",
  "  [0]-[9]        - Digit buttons",
  "  [-]            - [*] button",
  "  [=]            - [#] button",
  "  [PGUP]         - Fast-forward emulation (also: [F9])",
  "  [ESC]          - Quit emulation (also: [F12])",
#if defined(DEBUG)
  "  [F1]           - Go into the built-in debugger",
#endif
  "  [F2]           - Turn soundtrack log on/off",
  "  [F3]           - Toggle FIRE-R autofire on/off",
  "  [F4]           - Toggle FIRE-L autofire on/off",
  "  [F5]           - Invoke configuration menu",
  "  [F6]           - Load emulation state",
  "  [F7]           - Save emulation state",
  "  [F8]           - Replay recorded gameplay",
  "  [SHIFT]+[F8]   - Toggle gameplay recorder on/off",
  "  [F9]           - Fast-forward emulation (also: [PGUP])",
#if defined(GIFLIB)
  "  [F10]          - Make a screen snapshot (SNAPxxxx.GIF)",
#endif
  "  [F11]          - Reset hardware",
  "  [F12]          - Quit emulation (also: [ESC])",
  "  [ALT]+[PGUP]   - Increase audio volume",
  "  [ALT]+[PGDOWN] - Decrease audio volume",
#if defined(WINDOWS)
  "  [ALT]+[ENTER]  - Switch between full scren and window modes",
#endif
  0
};
