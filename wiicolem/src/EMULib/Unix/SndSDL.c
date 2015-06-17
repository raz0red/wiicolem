/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         SndSDL.c                        **/
/**                                                         **/
/** This file contains SDL-dependent sound implementation   **/
/** for the emulation library.                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"

#include "SDL.h"

static int SndRate     = 0;  /* Audio sampling rate          */
static int SndSize     = 0;  /* SndData[] size               */
static sample *SndData = 0;  /* Audio buffers                */
static int RPtr        = 0;  /* Read pointer into Bufs       */
static int WPtr        = 0;  /* Write pointer into Bufs      */
static volatile AudioPaused = 0; /* 1: Audio paused          */

/** AudioHandler() *******************************************/
/** Callback invoked by SDL to play audio.                  **/
/*************************************************************/
void AudioHandler(void *UserData,sample *Stream,int Length)
{
  int J;

  /* Need to have valid playback rate */
  if(!SndRate) return;

  /* Recompute length in samples */
  Length /= sizeof(sample);

  /* Copy audio data */
  for(J=0;J<Length;++J)
  {
    Stream[J] = SndData[RPtr];
    RPtr      = RPtr<SndSize-1? RPtr+1:0;
  }
}

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency)
{
  SDL_AudioSpec AudioFormat;
  int I,J,K;

  /* Shut down audio, just to be sure */
  TrashAudio();
  SndRate     = 0;
  SndSize     = 0;
  SndData     = 0;
  RPtr        = 0;
  WPtr        = 0;
  AudioPaused = 0;

  /* Have to have at least 8kHz sampling rate and 1ms buffer */
  if((Rate<8000)||!Latency) return(0);

  /* Compute number of sound buffers */
  SndSize=Rate*Latency/1000;

  /* Allocate audio buffers */
  SndData=(sample *)malloc(SndSize*sizeof(sample));
  if(!SndData) return(0);

  /* Set SDL audio settings */
  AudioFormat.freq     = Rate;
  AudioFormat.format   = sizeof(sample)>1? AUDIO_S16:AUDIO_S8;
  AudioFormat.channels = 1;
  AudioFormat.samples  = SndSize;
  AudioFormat.callback = AudioHandler;
  AudioFormat.userdata = 0;

  /* Open SDL audio device */
  if(SDL_OpenAudio(&AudioFormat,0)<0) { free(SndData);SndData=0;return(0); }

  /* Clear audio buffers */
  for(J=0;J<SndSize;++J) SndData[J]=0;

  /* Callback expects valid SndRate!=0 at the start */
  SndRate=Rate;

  /* Start playing SDL audio */
  SDL_PauseAudio(0);

  /* Done, return effective audio rate */
  return(SndRate);
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
  /* Sound off, pause off */
  SndRate     = 0;
  AudioPaused = 0;

  /* Audio off */
  SDL_CloseAudio();

  /* If buffers were allocated... */
  if(SndData) free(SndData);

  /* Sound trashed */
  SndData = 0;
  SndSize = 0;
  RPtr    = 0;
  WPtr    = 0;
}

/** PauseAudio() *********************************************/
/** Pause/resume audio playback.                            **/
/*************************************************************/
int PauseAudio(int Switch)
{
  /* Toggle audio status if requested */
  if(Switch==2) Switch=AudioPaused? 0:1;

  /* When switching audio state... */
  if((Switch>=0)&&(Switch<=1)&&(Switch!=AudioPaused))
  {
    /* Pause/Resume SDL audio */
    SDL_PauseAudio(Switch);
    /* Audio switched */
    AudioPaused=Switch;
  }

  /* Return current status */
  return(AudioPaused);
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{
  return(!SndRate? 0:RPtr>=WPtr? RPtr-WPtr:RPtr-WPtr+SndSize);
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
  unsigned int J;

  /* Require audio to be initialized */
  if(!SndRate) return(0);

  /* Copy audio samples */
  for(J=0;(J<Length)&&(RPtr!=WPtr);++J)
  {
    SndData[WPtr++]=Data[J];
    if(WPtr>=SndSize) WPtr=0;
  }

  /* Return number of samples copied */
  return(J);
}
