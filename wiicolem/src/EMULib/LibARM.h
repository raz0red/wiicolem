/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibARM.h                         **/
/**                                                         **/
/** This file declares optimized ARM assembler functions    **/
/** used to copy and process images on ARM-based platforms  **/
/** such as Symbian/S60, Symbian/UIQ, and Maemo. See files  **/
/** LibARM.asm (ARMSDT) and LibARM.s (GNU AS) for the       **/
/** actual functions.                                       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef LIBARM_H
#define LIBARM_H
#if defined(S60) || defined(UIQ) || defined(MAEMO)

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

void C256T120(pixel *Dst,const pixel *Src,int Count);
void C256T160(pixel *Dst,const pixel *Src,int Count);
void C256T176(pixel *Dst,const pixel *Src,int Count);
void C256T208(pixel *Dst,const pixel *Src,int Count);
void C256T240(pixel *Dst,const pixel *Src,int Count);
void C256T256(pixel *Dst,const pixel *Src,int Count);
void C256T320(pixel *Dst,const pixel *Src,int Count);
void C256T352(pixel *Dst,const pixel *Src,int Count);
void C256T416(pixel *Dst,const pixel *Src,int Count);
void C256T512(pixel *Dst,const pixel *Src,int Count);
void C256T768(pixel *Dst,const pixel *Src,int Count);
void TELEVIZE0(pixel *Dst,int Count);
void TELEVIZE1(pixel *Dst,int Count);

#ifdef __cplusplus
}
#endif
#endif /* S60 || UIQ || MAEMO */
#endif /* LIBARM_H */
