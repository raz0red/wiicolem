/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SN76489.c                        **/
/**                                                         **/
/** This file contains emulation for the SN76489 sound chip **/
/** produced by Intel. See SN76489.h for declarations.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "SN76489.h"
#include "Sound.h"

static const int Volumes[16] =
{ 255,203,161,128,102,81,64,51,40,32,25,20,16,13,10,0 };

static const int NoiseOUT[4] = { 14,15,14,14 };
static const int NoiseXOR[4] = { 13,12,10,13 };

/** Reset76489() *********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset76489(SN76489 *D,int ClockHz,int First)
{
  register int J;

  for(J=0;J<SN76489_CHANNELS;J++) D->Volume[J]=D->Freq[J]=0;

  D->Clock     = ClockHz>>5;
  D->NoiseMode = (First&SN76489_MODE)|0x07;
  D->Buf       = 0x00;
  D->Changed   = 0x80|((1<<SN76489_CHANNELS)-1);
  D->Sync      = SN76489_ASYNC;
  First       &= ~SN76489_MODE;
  D->First     = First;

  /* Set instruments */
  SetSound(0+First,SND_MELODIC);
  SetSound(1+First,SND_MELODIC);
  SetSound(2+First,SND_MELODIC);
  SetSound(3+First,SND_NOISE);
}

/** Sync76489() **********************************************/
/** Flush all accumulated changes by issuing Sound() calls, **/
/** and set the synchronization on/off. The second argument **/
/** should be SN76489_SYNC, SN76489_ASYNC to set/reset sync **/
/** or SN76489_FLUSH to leave sync mode as it is. To play   **/
/** noise channel with MIDI drums, OR second argument with  **/
/** SN76489_DRUMS.                                          **/
/*************************************************************/
void Sync76489(SN76489 *D,byte Sync)
{
  register int J,I;

  /* Hit MIDI drums for noise channels, if requested */
  if(Sync&SN76489_DRUMS)
  {
    if(D->Volume[3]&&D->Freq[3]) Drum(DRM_MIDI|28,D->Volume[3]);
    Sync&=~SN76489_DRUMS;
  }

  /* Set sync mode, if requested */
  if(Sync!=SN76489_FLUSH) D->Sync=Sync;

  /* Update noise generator parameters */
  if(D->Changed&0x80)
  {
    J=D->NoiseMode>>6; 
    SetNoise(0x0001,NoiseOUT[J],D->NoiseMode&0x04? NoiseXOR[J]:NoiseOUT[J]+1);
    D->Changed&=0x7F;
  }

  /* Update channels */
  for(J=0,I=D->Changed;I&&(J<SN76489_CHANNELS);++J,I>>=1)
    if(I&1) Sound(J+D->First,D->Freq[J],D->Volume[J]);

  /* Done */
  D->Changed=0x00;
}

/** Write76489() *********************************************/
/** Call this function to output a value V into the sound   **/
/** chip.                                                   **/
/*************************************************************/
void Write76489(SN76489 *D,byte V)
{
  register byte N,J;
  register long L;

  switch(V&0xF0)
  {
    case 0xE0:
      /* Keep track of noise generator changes */
      if((V^D->NoiseMode)&0x04) D->Changed|=0x80;
      /* Set noise period */
      if((V^D->NoiseMode)&0x03)
      {
        J=V&0x03;
        D->Freq[3]=J==0x03? D->Freq[2]:D->Clock/(0x10<<J);
        D->Changed|=0x08;
      }
      D->NoiseMode=(D->NoiseMode&~0x07)|(V&0x07);
      break;

    case 0x80: case 0xA0: case 0xC0:
      D->Buf=V;
      break;

    case 0x90: case 0xB0: case 0xD0: case 0xF0:
      N=(V-0x90)>>5;
      J=Volumes[V&0x0F];
      if(J!=D->Volume[N])
      {
        D->Volume[N]=J;
        D->Changed|=1<<N;
      }
      break;

    default:
      if(!(D->Buf&0xC0)) return;
      N=(D->Buf-0x80)>>5;
      L=D->Clock/(((int)(V&0x3F)<<4)+(D->Buf&0x0F)+1);
      /* Do not put upper limit on frequency here, since some */
      /* games like it >15kHz (dynamite sound in Coleco HERO) */
      if(L!=D->Freq[N])
      {
        if((N==2)&&((D->NoiseMode&0x03)==0x03))
        {
          D->Freq[3]=L;
          D->Changed|=0x08;
        }
        D->Freq[N]=L;
        D->Changed|=1<<N;
      }  
      break;
  }

  /* For asynchronous mode, make Sound() calls right away */
  if(!D->Sync&&D->Changed) Sync76489(D,SN76489_FLUSH);
}
