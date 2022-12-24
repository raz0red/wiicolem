/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        DRV9918.c                        **/
/**                                                         **/
/** This file contains screen refresh routines for the      **/
/** Texas Instruments TMS9918 video chip emulation. See     **/
/** TMS9918.h for declarations and TMS9918.c for the main   **/
/** code.                                                   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "TMS9918.h"

/** Static Functions *****************************************/
/** Functions used internally by the TMS9918 drivers.       **/
/*************************************************************/
static void RefreshSprites(TMS9918 *VDP,register byte Y);
static void RefreshBorder(TMS9918 *VDP,register byte Y);

/** RefreshBorder() ******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the screen border.                                      **/
/*************************************************************/
void RefreshBorder(register TMS9918 *VDP,register byte Y)
{
  register pixel *P,BC;
  register int J,N;

  /* Border color */
  BC=VDP->BGColor;

  /* Screen buffer */
  P=(pixel *)(VDP->XBuf);
  J=VDP->Width*(Y+(VDP->Height-192)/2);

  /* For the first line, refresh top border */
  if(Y) P+=J;
  else for(;J;J--) *P++=BC;

  /* Calculate number of pixels */
  N=(VDP->Width-(VDP->Mode? 256:240))/2; 

  /* Refresh left border */
  for(J=N;J;J--) *P++=BC;

  /* Refresh right border */
  P+=VDP->Width-(N<<1);
  for(J=N;J;J--) *P++=BC;

  /* For the last line, refresh bottom border */
  if(Y==191)
    for(J=VDP->Width*(VDP->Height-192)/2;J;J--) *P++=BC;
}

/** RefreshSprites() *****************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** sprites.                                                **/
/*************************************************************/
void RefreshSprites(register TMS9918 *VDP,register byte Y)
{
  static const byte SprHeights[4] = { 8,16,16,32 };
  register byte OH,IH,*PT,*AT;
  register pixel *P,*T,C;
  register int L,K,N;
  unsigned int M;

  /* Find sprites to show, update 5th sprite status */
  N = ScanSprites(VDP,Y,&M);
  if((N<0) || !M) return;

  T  = (pixel *)(VDP->XBuf)
     + VDP->Width*(Y+(VDP->Height-192)/2)
     + VDP->Width/2-128;
  OH = SprHeights[VDP->R[1]&0x03];
  IH = SprHeights[VDP->R[1]&0x02];
  AT = VDP->SprTab+(N<<2);

  /* For each possibly shown sprite... */
  for( ; N>=0 ; --N, AT-=4)
  {
    /* If showing this sprite... */
    if(M&(1<<N))
    {
      C=AT[3];                  /* C = sprite attributes */
      L=C&0x80? AT[1]-32:AT[1]; /* Sprite may be shifted left by 32 */
      C&=0x0F;                  /* C = sprite color */

      if((L<256)&&(L>-OH)&&C)
      {
        K=AT[0];                /* K = sprite Y coordinate */
        if(K>256-IH) K-=256;    /* Y coordinate may be negative */

        C  = VDP->XPal[C];
        P  = T+L;
        K  = Y-K-1;
        PT = VDP->SprGen
           + ((int)(IH>8? (AT[2]&0xFC):AT[2])<<3)
           + (OH>IH? (K>>1):K);

        /* Mask 1: clip left sprite boundary */
        K=L>=0? 0xFFFF:(0x10000>>(OH>IH? (-L>>1):-L))-1;

        /* Mask 2: clip right sprite boundary */
        L+=(int)OH-257;
        if(L>=0)
        {
          L=(IH>8? 0x0002:0x0200)<<(OH>IH? (L>>1):L);
          K&=~(L-1);
        }

        /* Get and clip the sprite data */
        K&=((int)PT[0]<<8)|(IH>8? PT[16]:0x00);

        if(OH>IH)
        {
          /* Big (zoomed) sprite */

          /* Draw left 16 pixels of the sprite */
          if(K&0xFF00)
          {
            if(K&0x8000) P[1]=P[0]=C;
            if(K&0x4000) P[3]=P[2]=C;
            if(K&0x2000) P[5]=P[4]=C;
            if(K&0x1000) P[7]=P[6]=C;
            if(K&0x0800) P[9]=P[8]=C;
            if(K&0x0400) P[11]=P[10]=C;
            if(K&0x0200) P[13]=P[12]=C;
            if(K&0x0100) P[15]=P[14]=C;
          }

          /* Draw right 16 pixels of the sprite */
          if(K&0x00FF)
          {
            if(K&0x0080) P[17]=P[16]=C;
            if(K&0x0040) P[19]=P[18]=C;
            if(K&0x0020) P[21]=P[20]=C;
            if(K&0x0010) P[23]=P[22]=C;
            if(K&0x0008) P[25]=P[24]=C;
            if(K&0x0004) P[27]=P[26]=C;
            if(K&0x0002) P[29]=P[28]=C;
            if(K&0x0001) P[31]=P[30]=C;
          }
        }
        else
        {
          /* Normal (unzoomed) sprite */

          /* Draw left 8 pixels of the sprite */
          if(K&0xFF00)
          {
            if(K&0x8000) P[0]=C;
            if(K&0x4000) P[1]=C;
            if(K&0x2000) P[2]=C;
            if(K&0x1000) P[3]=C;
            if(K&0x0800) P[4]=C;
            if(K&0x0400) P[5]=C;
            if(K&0x0200) P[6]=C;
            if(K&0x0100) P[7]=C;
          }

          /* Draw right 8 pixels of the sprite */
          if(K&0x00FF)
          {
            if(K&0x0080) P[8]=C;
            if(K&0x0040) P[9]=C;
            if(K&0x0020) P[10]=C;
            if(K&0x0010) P[11]=C;
            if(K&0x0008) P[12]=C;
            if(K&0x0004) P[13]=C;
            if(K&0x0002) P[14]=C;
            if(K&0x0001) P[15]=C;
          }
        }
      }
    }
  }
}

/** RefreshLine0() *******************************************/
/** Refresh line Y (0..191) of SCREEN0, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine0(register TMS9918 *VDP,register byte Y)
{
  register byte *T,X,K,Offset;
  register pixel *P,FC,BC;

  P  = (pixel *)(VDP->XBuf)
     + VDP->Width*(Y+(VDP->Height-192)/2)
     + VDP->Width/2-128+8;
  BC = VDP->BGColor;
  FC = VDP->FGColor;

  if(!TMS9918_ScreenON(VDP))
    for(X=0;X<240;X++) *P++=BC;
  else
  {
    T=VDP->ChrTab+(Y>>3)*40;
    Offset=Y&0x07;

    for(X=0;X<40;X++)
    {
      K=VDP->ChrGen[((int)*T<<3)+Offset];
      P[0]=K&0x80? FC:BC;
      P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;
      P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;
      P[5]=K&0x04? FC:BC;
      P+=6;T++;
    }
  }

  /* Refresh screen border */
  RefreshBorder(VDP,Y);
}

/** RefreshLine1() *******************************************/
/** Refresh line Y (0..191) of SCREEN1, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine1(register TMS9918 *VDP,register byte Y)
{
  register byte *T,X,K,Offset;
  register pixel *P,FC,BC;

  P  = (pixel *)(VDP->XBuf)
     + VDP->Width*(Y+(VDP->Height-192)/2)
     + VDP->Width/2-128;

  if(!TMS9918_ScreenON(VDP))
  {
    register int J;
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    T=VDP->ChrTab+((int)(Y&0xF8)<<2);
    Offset=Y&0x07;

    for(X=0;X<32;X++)
    {
      K=*T;
      BC=VDP->ColTab[K>>3];
      FC=VDP->XPal[BC>>4];
      BC=VDP->XPal[BC&0x0F];
      K=VDP->ChrGen[((int)K<<3)+Offset];
      P[0]=K&0x80? FC:BC;
      P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;
      P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;
      P[5]=K&0x04? FC:BC; 
      P[6]=K&0x02? FC:BC;
      P[7]=K&0x01? FC:BC;
      P+=8;T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP,Y);
  }

  /* Refresh screen border */
  RefreshBorder(VDP,Y);
}

/** RefreshLine2() *******************************************/
/** Refresh line Y (0..191) of SCREEN2, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine2(register TMS9918 *VDP,register byte Y)
{
  register pixel *P,FC,BC;
  register byte X,K,*T,*PGT,*CLT;
  register int J,I,PGTMask,CLTMask;

  P  = (pixel *)(VDP->XBuf)
     + VDP->Width*(Y+(VDP->Height-192)/2)
     + VDP->Width/2-128;

  if(!TMS9918_ScreenON(VDP))
  {
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    J       = ((int)(Y&0xC0)<<5)+(Y&0x07);
    PGT     = VDP->ChrGen;
    CLT     = VDP->ColTab;
    PGTMask = VDP->ChrGenM;
    CLTMask = VDP->ColTabM;
    T       = VDP->ChrTab+((int)(Y&0xF8)<<2);

    for(X=0;X<32;X++)
    {
      I    = (int)*T<<3;
      K    = CLT[(J+I)&CLTMask];
      FC   = VDP->XPal[K>>4];
      BC   = VDP->XPal[K&0x0F];
      K    = PGT[(J+I)&PGTMask];
      P[0] = K&0x80? FC:BC;
      P[1] = K&0x40? FC:BC;
      P[2] = K&0x20? FC:BC;
      P[3] = K&0x10? FC:BC;
      P[4] = K&0x08? FC:BC;
      P[5] = K&0x04? FC:BC;
      P[6] = K&0x02? FC:BC;
      P[7] = K&0x01? FC:BC;
      P+=8;
      T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP,Y);
  }

  /* Refresh screen border */
  RefreshBorder(VDP,Y);
}

/** RefreshLine3() *******************************************/
/** Refresh line Y (0..191) of SCREEN3, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine3(register TMS9918 *VDP,register byte Y)
{
  register byte *T,X,K,Offset;
  register pixel *P;

  P  = (pixel *)(VDP->XBuf)
     + VDP->Width*(Y+(VDP->Height-192)/2)
     + VDP->Width/2-128;

  if(!TMS9918_ScreenON(VDP))
  {
    register pixel BC;
    register int J;
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    T=VDP->ChrTab+((int)(Y&0xF8)<<2);
    Offset=(Y&0x1C)>>2;

    for(X=0;X<32;X++)
    {
      K=VDP->ChrGen[((int)*T<<3)+Offset];
      P[0]=P[1]=P[2]=P[3]=VDP->XPal[K>>4];
      P[4]=P[5]=P[6]=P[7]=VDP->XPal[K&0x0F];
      P+=8;T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP,Y);
  }

  /* Refresh screen border */
  RefreshBorder(VDP,Y);
}
