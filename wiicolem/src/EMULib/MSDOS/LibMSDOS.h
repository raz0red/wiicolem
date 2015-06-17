/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                      LibMSDOS.h                         **/
/**                                                         **/
/** This file contains MSDOS-dependent definitions and      **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef LIBMSDOS_H
#define LIBMSDOS_H

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BPS8                /* Using 8bit sound on MSDOS     */

#define SND_CHANNELS    16     /* Number of sound channels   */
#define OPL_CHANNELS    7      /* Number of Adlib channels   */
#define SND_SAMPLESIZE  256    /* Max. SetWave() sample size */
#define SND_BUFSIZE     512    /* Buffer size for DMA        */
#define SND_MAXDELAY    10     /* Maximal sound delay 1/n s  */

                               /* VGASetEffects() arguments  */
#define EFF_NONE        0x00   /* No special effects         */
#define EFF_SCALE       0x01   /* Scale video to fill screen */
#define EFF_SOFTEN      0x02   /* Scale + soften video       */
#define EFF_TVLINES     0x04   /* Apply TV scanlines effect  */
#define EFF_VSYNC       0x08   /* Wait for VBlanks           */
#define EFF_SYNC        0x10   /* Wait for sync timer        */

/** PIXEL() **************************************************/
/** MSDOS uses the 15bpp 5:5:5 pixel format.                **/
/*************************************************************/
#ifdef BPP16
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

/** InitMSDOS() **********************************************/
/** Initialize MSDOS keyboard handler, etc.                 **/
/*************************************************************/
void InitMSDOS(void);

/** TrashMSDOS() *********************************************/
/** Restore MSDOS keyboard handler, etc.                    **/
/*************************************************************/
void TrashMSDOS(void);

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

/** VGASetEffects() ******************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void VGASetEffects(int NewEffects);

/** VGAWaitVBlank() ******************************************/
/** Wait VGA VBlank.                                        **/
/*************************************************************/
void VGAWaitVBlank(void);

/** VGAText80() **********************************************/
/** Set 80x25 text screenmode.                              **/
/*************************************************************/
void VGAText80(void);

/** VGA320x200() *********************************************/
/** Set 320x200x256 screenmode.                             **/
/*************************************************************/
void VGA320x200(void);

/** VGA640x480() *********************************************/
/** Set 640x480 screenmode.                                 **/
/*************************************************************/
void VGA640x480(void);

/** VGA800x600() *********************************************/
/** Set 800x600x256 screenmode.                             **/
/*************************************************************/
void VGA800x600(void);

/** VGA256x240() *********************************************/
/** Set 256x240x256 screenmode.                             **/
/*************************************************************/
void VGA256x240(void);

/** VESASetMode() ********************************************/
/** Set VESA screen mode.                                   **/
/*************************************************************/
void VESASetMode(int Mode);

#ifdef __cplusplus
}
#endif
#endif /* LIBMSDOS_H */
