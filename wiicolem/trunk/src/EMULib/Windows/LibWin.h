/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibWin.h                         **/
/**                                                         **/
/** This file contains Windows-dependent definitions and    **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef LIBWIN_H
#define LIBWIN_H

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define BPP16    /* Pixels are 16bit values in 5:5:5 format  */
#define BPS16    /* Samples are 16bit signed values          */

#define HOMEPAGE      "http://fms.komkon.org/"

#define ASK_SAVEFILE  0x01
#define ASK_ASSOCIATE 0x02

#define SND_MINBLOCK    64
#define SND_HEADERS     64
#define SND_CHANNELS    16     /* Number of channels         */
#define SND_SAMPLESIZE  256    /* Max. SetWave() sample size */
#define SND_BUFSIZE     256    /* Size of a wave buffer      */
#define SND_BUFFERS     32     /* Number of wave buffers     */

#define MODE_640x480  -2
#define MODE_320x240  -1

/** WMENU_* **************************************************/
/** Default menu actions supported by LibWin.               **/
/*************************************************************/
#define WMENU_QUIT     900
#define WMENU_SIZEx1   901
#define WMENU_SIZEx2   902
#define WMENU_SIZEx3   903
#define WMENU_SIZEx4   904
#define WMENU_FULL     905
#define WMENU_SNDOFF   906 /* Group */
#define WMENU_SNDMIDI  907
#define WMENU_SND11kHz 908
#define WMENU_SND22kHz 909
#define WMENU_SND44kHz 910
#define WMENU_SND10ms  912 /* Group */
#define WMENU_SND50ms  913
#define WMENU_SND100ms 914
#define WMENU_SND250ms 915
#define WMENU_SND500ms 916
#define WMENU_SAVECPU  918
#define WMENU_SNDLOG   919
#define WMENU_SaveMID  920
#define WMENU_HELP     921
#define WMENU_WEB      922
#define WMENU_AUTHOR   923
#define WMENU_NETPLAY  924
#define WMENU_SOFTEN   925
#define WMENU_TV       926
#define WMENU_SYNC     927

/** PIXEL() **************************************************/
/** Windows uses the 15bpp 5:5:5 pixel format.              **/
/*************************************************************/
#if defined(BPP24) || defined(BPP32)
#define PIXEL(R,G,B)  (pixel)(((B)<<16)|((G)<<8)|(R))
#define PIXEL2MONO(P) (85*(((P)&255)+(((P)>>8)&255)+(((P)>>16)&255))>>8)
#define RMASK 0x000000FF
#define GMASK 0x0000FF00
#define BMASK 0x00FF0000
#elif defined(BPP16)
#define PIXEL(R,G,B)  (pixel)(((31*(R)/255)<<10)|((31*(G)/255)<<5)|(31*(B)/255))
#define PIXEL2MONO(P) (702*(((P)&31)+(((P)>>5)&31)+(((P)>>10)&31))>>8)
#define RMASK 0x7C00
#define GMASK 0x03E0
#define BMASK 0x001F
#else
#define PIXEL(R,G,B)  (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define PIXEL2MONO(P) (3264*((((P)<<1)&7)+(((P)>>2)&7)+(((P)>>5)&7))>>8)
#define RMASK 0xE0
#define GMASK 0x1C
#define BMASK 0x03
#endif

/** Application Variables ************************************/
/** Please define initial application appearance via these  **/
/** variables.                                              **/
/************************************ TO BE WRITTEN BY USER **/
extern const char *AppTitle;
extern const char *AppClass;
extern int AppWidth;
extern int AppHeight;
extern int AppMode;
extern int AppX;                   /* These two are provided */
extern int AppY;

/** Application() ********************************************/
/** This is the main application function you implement. It **/
/** is called from WinMain() after initializing things.     **/
/************************************ TO BE WRITTEN BY USER **/
int Application(int argc,char *argv[]);

/** WinGetAppInstance() **************************************/
/** Get application instance.                               **/
/*************************************************************/
HINSTANCE WinGetAppInstance(void);

/** WinGetAppWindow() ****************************************/
/** Get main window handle.                                 **/
/*************************************************************/
HWND WinGetAppWindow(void);

/** WinSetCmdHandler() ***************************************/
/** Attach a handler that will be called on menu selections **/
/** and other commands.                                     **/
/*************************************************************/
void WinSetCmdHandler(void (*Handler)(unsigned int Cmd));

/** WinProcessMsgs() *****************************************/
/** Call this function periodically to process Windows      **/
/** messages. Returns number of messages processed.         **/
/*************************************************************/
int WinProcessMsgs(void);

/** WinSetWindow() *******************************************/
/** Set window size and position. Returns 0 if failed.      **/
/*************************************************************/
int WinSetWindow(int X,int Y,int Width,int Height);

/** WinAskFilename() *****************************************/
/** Shows an open-file dialog and makes user select a file. **/
/** Returns 0 on failure. ATTENTION: This function returns  **/
/** pointer to the internal buffer!                         **/
/*************************************************************/
const char *WinAskFilename(const char *Title,const char *Types,unsigned int Options);

/** WinGetInteger() ******************************************/
/** Get integer setting by name or return Default if not    **/
/** found.                                                  **/
/*************************************************************/
unsigned int WinGetInteger(const char *Section,const char *Name,unsigned int Default);

/** WinSetInteger() ******************************************/
/** Set integer setting to a given value.                   **/
/*************************************************************/
void WinSetInteger(const char *Section,const char *Name,unsigned int Value);

/** WinGetString() *******************************************/
/** Get string setting by name.                             **/
/*************************************************************/
char *WinGetString(const char *Section,const char *Name,char *Value,int MaxChars);

/** WinSetString() *******************************************/
/** Set string setting to a given value.                    **/
/*************************************************************/
void WinSetString(const char *Section,const char *Name,const char *Value);

/** WinGetJoysticks() ****************************************/
/** Get the state of *real* joypad buttons (1="pressed"),   **/
/** but *not* the keyboard keys. Refer to BTN_* #defines    **/
/** for the button mappings.                                **/
/*************************************************************/
unsigned int WinGetJoysticks(void);

/** WinSetJoysticks() ****************************************/
/** Shows a dialog to set joystick button mappings.          */
/*************************************************************/
void WinSetJoysticks(unsigned int Buttons);

/** WinAssociate() *******************************************/
/** Associates files with given extension to the application */
/*************************************************************/
int WinAssociate(const char *Ext,const char *Comment,int Icon);

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent). The     **/
/** Delay value is given in millseconds and determines how  **/
/** much buffering will be done.                            **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Delay);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

/** WinInitSound() *******************************************/
/** Initialize Windows sound driver with given synthesis    **/
/** rate. Returns Rate on success, 0 otherwise. Pass Rate=0 **/
/** to skip initialization and be silent. Pass Rate=1 to    **/
/** use MIDI (midiOut). Pass Rate=8192..44100 to use wave   **/
/** synthesis (waveOut). Number of wave synthesis buffers   **/
/** must be in 2..SND_BUFFERS range.                        **/
/*************************************************************/
unsigned int WinInitSound(unsigned int Rate,unsigned int Delay);

/** WinTrashSound() ******************************************/
/** Free resources allocated by WinInitSound().             **/
/*************************************************************/
void WinTrashSound(void);

#ifdef __cplusplus
}
#endif
#endif /* LIBWIN_H */
