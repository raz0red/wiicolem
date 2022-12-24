/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        TMS9918.h                        **/
/**                                                         **/
/** This file contains emulation for the TMS9918 video chip **/
/** produced by Texas Instruments. See files TMS9918.c and  **/
/** DRV9918.c for implementation.                           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TMS9918_H
#define TMS9918_H
#ifdef __cplusplus
extern "C" {
#endif

#define TMS9918_BASE        10738635

#define TMS9918_FRAMES      60
#define TMS9918_LINES       262
#define TMS9918_CLOCK       (TMS9918_BASE/3)
#define TMS9918_FRAME       (TMS9918_BASE/(3*60))
#define TMS9918_LINE        (TMS9918_BASE/(3*60*262))

#define TMS9928_FRAMES      60
#define TMS9928_LINES       262
#define TMS9928_CLOCK       (TMS9918_BASE/3)
#define TMS9928_FRAME       (TMS9918_BASE/(3*60))
#define TMS9928_LINE        (TMS9918_BASE/(3*60*262))

#define TMS9929_FRAMES      50
#define TMS9929_LINES       313
#define TMS9929_CLOCK       (TMS9918_BASE/2)
#define TMS9929_FRAME       (TMS9918_BASE/(2*50))
#define TMS9929_LINE        (TMS9918_BASE/(2*50*313))

#define TMS9918_START_LINE  (3+13+27)
#define TMS9918_END_LINE    (TMS9918_START_LINE+192)

#define TMS9918_DRAWFRAMES  100  /* Default % frames to draw */
#define TMS9918_MAXSPRITES  4    /* Max number of sprites    */

#define TMS9918_STAT_VBLANK 0x80 /* 1: VBlank has occured    */
#define TMS9918_STAT_5THSPR 0x40 /* 1: 5th sprite detected   */
#define TMS9918_STAT_OVRLAP 0x20 /* 1: Sprites overlap       */
#define TMS9918_STAT_5THNUM 0x1F /* Number of the 5th sprite */

#define TMS9918_REG0_EXTVDP 0x01 /* 1: Enable external VDP   */

#define TMS9918_REG1_RAM16K 0x80 /* 1: 16kB VRAM (0=4kB)     */
#define TMS9918_REG1_SCREEN 0x40 /* 1: Enable display        */
#define TMS9918_REG1_IRQ    0x20 /* 1: IRQs on VBlanks       */
#define TMS9918_REG1_SPR16  0x02 /* 1: 16x16 sprites (0=8x8) */
#define TMS9918_REG1_BIGSPR 0x01 /* 1: Magnify sprites x2    */

#define TMS9918_Mode(VDP)      ((((VDP)->R[0]&0x02)>>1)|(((VDP)->R[1]&0x18)>>2))
#define TMS9918_ExtVDP(VDP)    ((VDP)->R[0]&TMS9918_REG0_EXTVDP)
#define TMS9918_VRAMMask(VDP)  ((VDP)->R[1]&TMS9918_REG1_RAM16K? 0x3FFF:0x0FFF)
#define TMS9918_ScreenON(VDP)  ((VDP)->R[1]&TMS9918_REG1_SCREEN)
#define TMS9918_VBlankON(VDP)  ((VDP)->R[1]&TMS9918_REG1_IRQ)
#define TMS9918_Sprites16(VDP) ((VDP)->R[1]&TMS9918_REG1_SPR16)
#define TMS9918_BigSprite(VDP) ((VDP)->R[1]&TMS9918_REG1_BIGSPR)

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

#ifndef QUAD_TYPE_DEFINED
#define QUAD_TYPE_DEFINED
typedef unsigned int quad;
#endif

#ifndef RGB_TYPE_DEFINED
#define RGB_TYPE_DEFINED
typedef struct { unsigned char R,G,B; } RGB;
#endif

#ifndef PIXEL_TYPE_DEFINED
#define PIXEL_TYPE_DEFINED
#if defined(BPP32) | defined(BPP24)
typedef unsigned int pixel;
#elif defined(BPP16)
typedef unsigned short pixel;
#elif defined(BPP8)
typedef unsigned char pixel;
#else
typedef unsigned int pixel;
#endif
#endif

/** TMS9918 **************************************************/
/** This data structure stores VDP state, screen buffer,    **/
/** VRAM, and other parameters.                             **/
/*************************************************************/
#pragma pack(4)
typedef struct
{
  /* User-Configurable Parameters */
  byte  DrawFrames;   /* % of frames to draw     */
  byte  MaxSprites;   /* Number of sprites/line  */
  int   Scanlines;    /* Scanlines per frame     */

  /* VDP State */
  quad  Reserved1;    /* Reserved, do not use    */
  byte  R[16];        /* VDP control registers   */
  byte  Status;       /* VDP status register     */
  byte  VKey;         /* 1: Control byte latched */
  byte  WKey;         /* 1: VRAM in WRITE mode   */
  byte  Mode;         /* Current screen mode     */
  int   Line;         /* Current scanline        */
  byte  CLatch;       /* Control register latch  */
  byte  DLatch;       /* Data register latch     */
  short VAddr;        /* VRAM access address     */
  quad  Reserved2[5]; /* Reserved, do not use    */

  /* Table Address Masks */
  int   ChrTabM;      /* ChrTab[] address mask   */
  int   ColTabM;      /* ColTab[] address mask   */
  int   ChrGenM;      /* ChrGen[] address mask   */
  int   SprTabM;      /* SprTab[] address mask   */

  /* Picture Rendering */
  int   Reserved3;    /* Reserved, do not use               */
  int   XPal[16];     /* Colors obtained with SetColor()    */
  int   FGColor;      /* Foreground color (Rg#7)            */
  int   BGColor;      /* Background color (Rg#7)            */
  byte  UCount;       /* Screen update counter              */
  byte  OwnXBuf;      /* 1: XBuf was allocated in New9918() */
  int   Width;        /* XBuf width in pixels >=256         */
  int   Height;       /* XBuf height in pixels >=192        */

  /*---- Save9918() will save all data up to this point ----*/

  void  *XBuf;        /* Output picture buffer   */
  byte  *VRAM;        /* 16kB of VRAM            */

  /* VRAM Tables */
  byte  *ChrTab;      /* Character Name Table    */
  byte  *ChrGen;      /* Character Pattern Table */
  byte  *SprTab;      /* Sprite Attribute Table  */
  byte  *SprGen;      /* Sprite Pattern Table    */
  byte  *ColTab;      /* Color Table             */
} TMS9918;
#pragma pack()

/** Palette9918[] ********************************************/
/** 16 standard colors used by TMS9918/TMS9928 VDP chips.   **/
/*************************************************************/
extern RGB Palette9918[16*3];

/** New9918() ************************************************/
/** Create a new VDP context. The user can either provide   **/
/** his own screen buffer by pointing Buffer to it, or ask  **/
/** New9918() to allocate a buffer by setting Buffer to 0.  **/
/** Width and Height must always specify screen buffer      **/
/** dimensions. New9918() returns pointer to the screen     **/
/** buffer on success, 0 otherwise.                         **/
/*************************************************************/
byte *New9918(TMS9918 *VDP,byte *Buffer,int Width,int Height);

/** Reset9918() **********************************************/
/** Reset the VDP. The user can provide a new screen buffer **/
/** by pointing Buffer to it and setting Width and Height.  **/
/** Set Buffer to 0 to use the existing screen buffer.      **/
/*************************************************************/
void Reset9918(TMS9918 *VDP,byte *Buffer,int Width,int Height);

/** Trash9918() **********************************************/
/** Free all buffers associated with VDP and invalidate VDP **/
/** context. Use this to shut down VDP.                     **/
/*************************************************************/
void Trash9918(TMS9918 *VDP);

/** WrCtrl9918() *********************************************/
/** Write a value V to the VDP Control Port. Enabling IRQs  **/
/** in this function may cause an IRQ to be generated. In   **/
/** this case, WrCtrl9918() returns 1. Returns 0 otherwise. **/
/*************************************************************/
byte WrCtrl9918(TMS9918 *VDP,byte V);

/** RdCtrl9918() *********************************************/
/** Read a value from the VDP Control Port.                 **/
/*************************************************************/
byte RdCtrl9918(TMS9918 *VDP);

/** WrData9918() *********************************************/
/** Write a value V to the VDP Data Port.                   **/
/*************************************************************/
void WrData9918(TMS9918 *VDP,byte V);

/** RdData9918() *********************************************/
/** Read a value from the VDP Data Port.                    **/
/*************************************************************/
byte RdData9918(TMS9918 *VDP);

/** Loop9918() ***********************************************/
/** Call this routine on every scanline to update the       **/
/** screen buffer. Loop9918() returns 1 if an interrupt is  **/
/** to be generated, 0 otherwise.                           **/
/*************************************************************/
byte Loop9918(TMS9918 *VDP);

/** Write9918() **********************************************/
/** This is a convinience function provided for the user to **/
/** write into VDP control registers. This can also be done **/
/** by two consequent WrCtrl9918() calls. Enabling IRQs in  **/
/** this function may cause an IRQ to be generated. In this **/
/** case, Write9918() returns 1. Returns 0 otherwise.       **/
/*************************************************************/
byte Write9918(TMS9918 *VDP,byte R,byte V);

/** Save9918() ***********************************************/
/** Save TMS9918 state to a given buffer of given maximal   **/
/** size. Returns number of bytes saved or 0 on failure.    **/
/*************************************************************/
unsigned int Save9918(const TMS9918 *VDP,byte *Buf,unsigned int Size);

/** Load9918() ***********************************************/
/** Load TMS9918 state from a given buffer of given maximal **/
/** size. Returns number of bytes loaded or 0 on failure.   **/
/*************************************************************/
unsigned int Load9918(TMS9918 *VDP,byte *Buf,unsigned int Size);

/** RefreshLine#() *******************************************/
/** Screen handlers refreshing current scanline in a screen **/
/** buffer. These functions are used from TMS9918.c and     **/
/** should not be called by the user.                       **/
/*************************************************************/
void RefreshLine0(TMS9918 *VDP,byte Y);
void RefreshLine1(TMS9918 *VDP,byte Y);
void RefreshLine2(TMS9918 *VDP,byte Y);
void RefreshLine3(TMS9918 *VDP,byte Y);

/** ScanSprites() ********************************************/
/** Compute bitmask of sprites shown in a given scanline.   **/
/** Returns the first sprite to show or -1 if none shown.   **/
/** Also updates 5th sprite fields in the status register.  **/
/*************************************************************/
int ScanSprites(TMS9918 *VDP,byte Y,unsigned int *Mask);

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshScreen(void *Buffer,int Width,int Height);

#ifdef __cplusplus
}
#endif
#endif /* TMS9918_H */
