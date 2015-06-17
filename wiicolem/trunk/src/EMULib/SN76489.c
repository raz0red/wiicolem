/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SN76489.c                        **/
/**                                                         **/
/** This file contains emulation for the SN76489 sound chip **/
/** produced by Intel. See SN76489.h for declarations.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "SN76489.h"
#include "Sound.h"

#ifdef WII
static byte FirstNoise = 1;
#endif

/** Reset76489() *********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset76489(SN76489 *D,int First)
{
  register int J;

  for(J=0;J<SN76489_CHANNELS;J++) D->Volume[J]=D->Freq[J]=0;

#ifdef WII
  FirstNoise = 1;
#endif
  D->NoiseMode = 0x00;
  D->Buf       = 0x00;
  D->Changed   = (1<<SN76489_CHANNELS)-1;
  D->Sync      = SN76489_ASYNC;
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

  if(Sync!=SN76489_FLUSH) D->Sync=Sync;

  for(J=0,I=D->Changed;I&&(J<SN76489_CHANNELS);J++,I>>=1)
    if(I&1) Sound(J+D->First,D->Freq[J],D->Volume[J]);

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
      J=V&0x03;
#ifdef WII
      if(J!=D->NoiseMode||FirstNoise)
      {
        FirstNoise=0;
#else
      if(J!=D->NoiseMode)
      {
#endif
        D->NoiseMode=J;
        switch(J)
        {
          case 0: D->Freq[3]=20000;break;
          case 1: D->Freq[3]=10000;break;
          case 2: D->Freq[3]=5000;break;
          case 3: D->Freq[3]=D->Freq[2];break;
        }
        D->Changed|=0x08;
      }
      break;

    case 0x80: case 0xA0: case 0xC0:
      D->Buf=V;
      break;

    case 0x90: case 0xB0: case 0xD0: case 0xF0:
      N=(V-0x90)>>5;
      J=(~V&0x0F)*16;
      if(J!=D->Volume[N])
      {
        D->Volume[N]=J;
        D->Changed|=1<<N;
      }
      break;

    default:
      if(!(D->Buf&0xC0)) return;
      N=(D->Buf-0x80)>>5;
      L=SN76489_BASE/((V&0x3F)*16+(D->Buf&0x0F)+1);
      if(L>15000) L=0;
      if(L!=D->Freq[N])
      {
        if((N==2)&&(D->NoiseMode==3))
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
