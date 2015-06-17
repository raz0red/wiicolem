/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibMaemo.h                       **/
/**                                                         **/
/** This file contains Maemo-dependent definitions and      **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef LIBMAEMO_H
#define LIBMAEMO_H

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define BPP16     /* Pixels are 16bit values in 5:5:5 format */
//#define BPS8      /* Samples are 8bit signed values          */

#define SND_CHANNELS    16     /* Number of sound channels   */
#define SND_BUFSIZE     256    /* Sound buffer size          */

                               /* GTKSetEffects() arguments  */
#define EFF_NONE        0x00   /* No special effects         */
#define EFF_SCALE       0x01   /* Scale video to fill screen */
#define EFF_SOFTEN      0x02   /* Scale + soften video       */
#define EFF_TVLINES     0x04   /* Apply TV scanlines effect  */
#define EFF_SAVECPU     0x08   /* Suspend when inactive      */
#define EFF_SYNC        0x10   /* Wait for sync timer        */
#define EFF_PENCUES     0x20   /* Draw pen input cue lines   */
#define EFF_DIALCUES    0x40   /* Draw dialpad (with PENCUES)*/
#define EFF_VERBOSE     0x80   /* Report problems via printf */
#define EFF_NOVOLUME    0x100  /* No volume control on F7/F8 */
#define EFF_SHOWFPS     0x200  /* Show frames-per-sec count  */
#define EFF_VSYNC       0x400  /* Use OMAPFB tearsync/vsync  */

                               /* Hardware key codes         */
#define CON_MENU        CON_F4 /* Menu button                */
#define CON_FULLSCR     CON_F6 /* Full screen toggle         */
#define CON_VOLUP       CON_F7 /* Volume increment           */
#define CON_VOLDOWN     CON_F8 /* Volume decrement           */

                               /* PenDialpad() result values */
#define DIAL_NONE          0   /* No dialpad buttons pressed */
#define DIAL_1             1
#define DIAL_2             2
#define DIAL_3             3
#define DIAL_4             4
#define DIAL_5             5
#define DIAL_6             6
#define DIAL_7             7
#define DIAL_8             8
#define DIAL_9             9
#define DIAL_STAR         10
#define DIAL_0            11
#define DIAL_POUND        12

/** PIXEL() **************************************************/
/** Unix uses the 16bpp 5:6:5 pixel format.                 **/
/*************************************************************/
#if defined(BPP32) || defined(BPP24)
#define PIXEL(R,G,B)  (pixel)(((int)R<<16)|((int)G<<8)|B)
#define PIXEL2MONO(P) (((P>>16)&0xFF)+((P>>8)&0xFF)+(P&0xFF))/3)
#define RMASK 0xFF0000
#define GMASK 0x00FF00
#define BMASK 0x0000FF

#elif defined(BPP16)
#define PIXEL(R,G,B)  (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#define PIXEL2MONO(P) (522*(((P)&31)+(((P)>>5)&63)+(((P)>>11)&31))>>8)
#define RMASK 0xF800
#define GMASK 0x07E0
#define BMASK 0x001F
#else

#define PIXEL(R,G,B)  (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define PIXEL2MONO(P) (3264*((((P)<<1)&7)+(((P)>>2)&7)+(((P)>>5)&7))>>8)
#define RMASK 0xE0
#define GMASK 0x1C
#define BMASK 0x03
#endif

extern int  ARGC;
extern char **ARGV;

/** InitMaemo() **********************************************/
/** Initialize Unix/GTK resources and set initial window    **/
/** title and dimensions.                                   **/
/*************************************************************/
int InitMaemo(const char *Title,int Width,int Height,const char *Service);

/** TrashMaemo() *********************************************/
/** Free resources allocated in InitMaemo()                 **/
/*************************************************************/
void TrashMaemo(void);

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

/** GTKProcessEvents() ***************************************/
/** Process GTK event messages.                             **/
/*************************************************************/
void GTKProcessEvents(void);

/** GTKSetEffects() ******************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void GTKSetEffects(int NewEffects);

/** GTKResizeVideo() *****************************************/
/** Change output rectangle dimensions.                     **/
/*************************************************************/
int GTKResizeVideo(int Width,int Height);

/** GTKAddMenu() *********************************************/
/** Add a menu with a command ID.                           **/
/*************************************************************/
int GTKAddMenu(const char *Label,int CmdID);

/** GTKAskFilename() *****************************************/
/** Ask user to select a file and return the file name, or  **/
/** 0 on cancel.                                            **/
/*************************************************************/
const char *GTKAskFilename(GtkFileChooserAction Action);

/** GTKMessage() *********************************************/
/** Show text message to the user.                          **/
/*************************************************************/
void GTKMessage(const char *Text);

/** GTKGet*()/GTKSet*() **************************************/
/** Wrappers for getting and setting GConf config values.   **/
/*************************************************************/
const char *GTKGetString(const char *Key);
unsigned int GTKGetInteger(const char *Key,unsigned int Default);

int GTKSetString(const char *Key,const char *Value);
int GTKSetInteger(const char *Key,unsigned int Value);

/** SetFileHandler() *****************************************/
/** Attach handler that will be called to open files.       **/
/*************************************************************/
void SetFileHandler(int (*Handler)(const char *Filename));

/** PenJoystick() ********************************************/
/** Get simulated joystick buttons from touch screen UI.    **/
/** Result compatible with GetJoystick().                   **/
/*************************************************************/
unsigned int PenJoystick(void);

/** PenDialpad() *********************************************/
/** Get simulated dialpad buttons from touch screen UI.     **/
/*************************************************************/
unsigned char PenDialpad(void);

#ifdef __cplusplus
}
#endif
#endif /* LIBMAEMO_H */
