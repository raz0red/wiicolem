/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SN76489.h                        **/
/**                                                         **/
/** This file contains emulation for the SN76489 sound chip **/
/** produced by Intel. See SN76489.c for the code.          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef SN76489_H
#define SN76489_H
#ifdef __cplusplus
extern "C" {
#endif

#define SN76489_BASE     111052 /* SN76489 base frequency    */
#define SN76489_CHANNELS 4      /* Total number of channels  */

#define SN76489_ASYNC    0      /* Asynchronous emulation    */
#define SN76489_SYNC     1      /* Synchronous emulation     */
#define SN76489_FLUSH    2      /* Flush buffers only        */
#define SN76489_DRUMS    0x80   /* Hit drums for noise chnl  */

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** SN76489 **************************************************/
/** This data structure stores SN76489 state.               **/
/*************************************************************/
typedef struct
{
  int Channel;                 /* Current channel */
  int Freq[SN76489_CHANNELS];  /* Frequencies (0 for off) */
  int Volume[SN76489_CHANNELS]; /* Volumes (0..255) */
  byte Sync;                   /* Sync mode */
  byte NoiseMode;              /* Noise mode */
  byte Buf;                    /* Latch to store a value */
  byte Changed;                /* Bitmap of changed channels */
  int First;                   /* First used Sound() channel */
} SN76489;

/** Reset76489() *********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset76489(register SN76489 *D,int First);

/** Sync76489() **********************************************/
/** Flush all accumulated changes by issuing Sound() calls, **/
/** and set the synchronization on/off. The second argument **/
/** should be SN76489_SYNC, SN76489_ASYNC to set/reset sync **/
/** or SN76489_FLUSH to leave sync mode as it is. To play   **/
/** noise channel with MIDI drums, OR second argument with  **/
/** SN76489_DRUMS.                                          **/
/*************************************************************/
void Sync76489(register SN76489 *D,register byte Sync);

/** Write76489() *********************************************/
/** Call this function to output a value V into the sound   **/
/** chip.                                                   **/
/*************************************************************/
void Write76489(register SN76489 *D,register byte V);

#ifdef __cplusplus
}
#endif
#endif /* SN76489_H */
