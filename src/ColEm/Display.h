/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                       Display.h                         **/
/**                                                         **/
/** This file instantiates TMS9918 display drivers for all  **/
/** supported screen depths.                                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

/** Screen9918[] *********************************************/
/** Pointer to the scanline refresh handlers and VDP table  **/
/** address masks for the standard TMS9918 screen modes.    **/
/*************************************************************/
extern struct
{
  void (*LineHandler)(TMS9918 *VDP,byte Y);
  unsigned char R2,R3,R4,R5,R6,M2,M3,M4,M5;
} Screen9918[4];

#define PIXEL_TYPE_DEFINED
#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32

#define BPP8
#define pixel unsigned char
#define RefreshSprites RefreshSprites_8
#define RefreshBorder  RefreshBorder_8
#define RefreshLine0   RefreshLine0_8
#define RefreshLine1   RefreshLine1_8
#define RefreshLine2   RefreshLine2_8
#define RefreshLine3   RefreshLine3_8
#include "DRV9918.c"
#undef BPP8
#undef pixel
#undef RefreshSprites
#undef RefreshBorder
#undef RefreshLine0
#undef RefreshLine1
#undef RefreshLine2
#undef RefreshLine3

#define BPP16
#define pixel unsigned short
#define RefreshSprites RefreshSprites_16
#define RefreshBorder  RefreshBorder_16
#define RefreshLine0   RefreshLine0_16
#define RefreshLine1   RefreshLine1_16
#define RefreshLine2   RefreshLine2_16
#define RefreshLine3   RefreshLine3_16
#include "DRV9918.c"
#undef BPP16
#undef pixel
#undef RefreshSprites
#undef RefreshBorder
#undef RefreshLine0
#undef RefreshLine1
#undef RefreshLine2
#undef RefreshLine3

#define BPP32
#define pixel unsigned int
#define RefreshSprites RefreshSprites_32
#define RefreshBorder  RefreshBorder_32
#define RefreshLine0   RefreshLine0_32
#define RefreshLine1   RefreshLine1_32
#define RefreshLine2   RefreshLine2_32
#define RefreshLine3   RefreshLine3_32
#include "DRV9918.c"
#undef BPP32
#undef pixel
#undef RefreshSprites
#undef RefreshBorder
#undef RefreshLine0
#undef RefreshLine1
#undef RefreshLine2
#undef RefreshLine3

/** SetScreenDepth() *****************************************/
/** Fill TMS9918 screen driver array with pointers matching **/
/** the given image depth.                                  **/
/*************************************************************/
int SetScreenDepth(int Depth)
{
  if(Depth<=8)
  {
    Depth=8;
    Screen9918[0].LineHandler = RefreshLine0_8;
    Screen9918[1].LineHandler = RefreshLine1_8;
    Screen9918[2].LineHandler = RefreshLine2_8;
    Screen9918[3].LineHandler = RefreshLine3_8;
  }
  else if(Depth<=16)
  {
    Depth=16;
    Screen9918[0].LineHandler = RefreshLine0_16;
    Screen9918[1].LineHandler = RefreshLine1_16;
    Screen9918[2].LineHandler = RefreshLine2_16;
    Screen9918[3].LineHandler = RefreshLine3_16;
  }
  else if(Depth<=32)
  {
    Depth=32;
    Screen9918[0].LineHandler = RefreshLine0_32;
    Screen9918[1].LineHandler = RefreshLine1_32;
    Screen9918[2].LineHandler = RefreshLine2_32;
    Screen9918[3].LineHandler = RefreshLine3_32;
  }
  else
  {
    /* Wrong depth */
    Depth=0;
  }

  return(Depth);
}

#endif /* DISPLAY_H */
