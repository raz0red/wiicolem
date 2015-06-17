/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          SndWin.c                       **/
/**                                                         **/
/** This file contains standard sound generation routines   **/
/** for Windows using wave synthesis or MIDI.               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"

/** SHIFT_CHANNEL() ******************************************/
/** Make MIDI channel#10 last, as it is normally used for   **/
/** percussion instruments only and doesn't sound nice.     **/
/*************************************************************/
#define SHIFT_CHANNEL(Ch) (Ch==15? 9:Ch>8? Ch+1:Ch)

static const struct { BYTE Note;WORD Wheel; } Freqs[4096] =
{
#include "MIDIFreq.h"
};

static int DrumOn;
static HMIDIOUT hMIDI = 0;
static int SndRate    = 0;
extern int MasterSwitch;
extern int MasterVolume;

static struct
{
  int Type;                       /* Channel type (SND_*)             */
  int Freq;                       /* Channel frequency (Hz)           */
  int Volume;                     /* Channel volume (0..255)          */

  int Note;                       /* MIDI note (-1 for off)           */
  int Pitch;                      /* MIDI pitch                       */
  int Level;                      /* MIDI volume level (velocity)     */
} CH[SND_CHANNELS] =
{
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 },
  { SND_MELODIC,0,0,-1,-1,-1 }
};

static void WinSetSound(int Channel,int Type);
static void WinDrum(int Type,int Force);
static void WinSetChannels(int Volume,int Switch);
static void WinSetWave(int Channel,const signed char *Data,int Length,int Rate);
static void WinSound(int Channel,int Freq,int Volume);
static void NoteOn(int Channel,int Note,int Level);
static void NoteOff(int Channel);

/** WinInitSound() *******************************************/
/** Initialize sound. Returns 0 on failure, effective rate  **/
/** otherwise. Special cases of rate argument: 0 - return 1 **/
/** and be silent, 1..8191 - use MIDI mapper to play music. **/
/*************************************************************/
unsigned int WinInitSound(unsigned int Rate,unsigned int Delay)
{
  int J;

  /* If initialized, shut down */
  WinTrashSound();

  /* Set driver functions */
  SndDriver.SetSound    = WinSetSound;
  SndDriver.Drum        = WinDrum;
  SndDriver.SetChannels = WinSetChannels;
  SndDriver.Sound       = WinSound;
  SndDriver.SetWave     = WinSetWave;
  SndDriver.GetWave     = 0;

  /* Reset channels */
  for(J=0;J<SND_CHANNELS;J++)
  {
    CH[J].Freq   = 0;
    CH[J].Volume = 0;
    CH[J].Note   = -1;
    CH[J].Pitch  = -1;
    CH[J].Level  = -1;
  }

  /* Reset variables */
  SndRate = 0;
  hMIDI   = 0;
  DrumOn  = 0;

  /* No sound requested: EXIT */
  if(!Rate) return(0);

  /* Reopen sound */
  if(Rate>=8192) SndRate=InitAudio(Rate,Delay);
  else
  {
    /* Open MIDI mapper */
    if(midiOutOpen(&hMIDI,MIDI_MAPPER,0,0,0)) return(0);
    /* midiOut opened successfully */
    SndRate=1;
    /* Set initial volumes */
    // for(J=0;J<SND_CHANNELS;J++) midiOutShortMsg(hMIDI,0x6407B0|J);
  }

  /* Fall out if failed */
  if(!SndRate) return(0);

  /* Set current instrments (if reopening device) */
  for(J=0;J<SND_CHANNELS;J++) WinSetSound(J,CH[J].Type);

  /* Done */
  return(SndRate);
}

/** WinTrashSound() ******************************************/
/** Free resources allocated by WinInitSound().             **/
/*************************************************************/
void WinTrashSound(void)
{
  /* Drop out if sound not initialized */
  if(!SndRate) return;

  /* Shutdown wave audio or close midiOut */
  if(SndRate>=8192) TrashAudio();
  else
    if(hMIDI)
    {
      midiOutReset(hMIDI);
      midiOutClose(hMIDI);
      hMIDI=0;
    }

  /* Done */
  SndRate=0;
}

/** WinSetSound() ********************************************/
/** Set sound type for a given channel.                     **/
/*************************************************************/
void WinSetSound(int Channel,int Type)
{
  static const int Programs[5] = { 80,80,122,122,80 };

  if(!SndRate||(Channel<0)||(Channel>=SND_CHANNELS)) return;

  CH[Channel].Type=Type;

  if(SndRate<8192)
  {
    Type=Type&SND_MIDI? Type&0x7F:Programs[Type&0x03];
    midiOutShortMsg(hMIDI,(Type<<8)|(SHIFT_CHANNEL(Channel)&0x0F)|0xC0);
  }
}

/** WinDrum() ************************************************/
/** Hit a drum of a given type with given force.            **/
/*************************************************************/
void WinDrum(int Type,int Force)
{
  if(!SndRate) return;
  Force=Force<0? 0:Force>255? 255:Force;

  if((SndRate<8192)&&(MasterSwitch&0x8000))
  {
    /* The only non-MIDI drum is a click ("Low Wood Block") */
    Type=Type&DRM_MIDI? Type&0x7F:77;
    /* Release current drum */
    if(DrumOn) midiOutShortMsg(hMIDI,(127<<16)|(DrumOn<<8)|0x89);
    /* Hit current drum */
    if(Type) midiOutShortMsg(hMIDI,(((Force+1)>>1)<<16)|(Type<<8)|0x99);
    DrumOn=Type;
  }
}

/** WinSound() ***********************************************/
/** Set sound volume and frequency for a given channel.     **/
/*************************************************************/
void WinSound(int Channel,int Freq,int Volume)
{
  int MIDIVolume,MIDINote,MIDIWheel;

  if(!SndRate||(Channel<0)||(Channel>=SND_CHANNELS)) return;
  Volume=Volume<0? 0:Volume>255? 255:Volume;

  if(SndRate<8192)
  {
    /* Clip the frequency */
    if((Freq<MIDI_MINFREQ)||(Freq>MIDI_MAXFREQ)) Freq=0;
    /* SND_TRIANGLE has 1/2 volume of SND_MELODIC */
    if(CH[Channel].Type==SND_TRIANGLE) Volume=(Volume+1)/2;
    /* Store frequency and volume */
    CH[Channel].Freq   = Freq;
    CH[Channel].Volume = Volume;

    if(MasterSwitch&(1<<Channel))
    {
      if(!Volume||!Freq) NoteOff(Channel);
      else
      {
        /* MIDI volume is 0..127, frequency is 0..4095 */
        MIDIVolume = (127*Volume+128)/255;
        MIDINote   = Freqs[Freq/3].Note;
        MIDIWheel  = Freqs[Freq/3].Wheel;

        /* Play new note */
        NoteOn(Channel,MIDINote,MIDIVolume);

        /* Change pitch */
        if(CH[Channel].Pitch!=MIDIWheel)
        {
          midiOutShortMsg
          (
            hMIDI,
            0xE0
            | (SHIFT_CHANNEL(Channel)&0x0F)
            | ((MIDIWheel&0x3F80)<<1)
            | ((MIDIWheel&0x7F)<<8)
          );
          CH[Channel].Pitch=MIDIWheel;
        }
      }
    }
  }
}

/** WinSetChannels() *****************************************/
/** Set overall sound volume and switch channels on/off.    **/
/*************************************************************/
void WinSetChannels(int Volume,int Switch)
{
  int J;

  if(!SndRate) return;
  MasterVolume = Volume<0? 0:Volume>255? 255:Volume;
  MasterSwitch = MasterSwitch&((1<<SND_CHANNELS)-1);

  if(SndRate<8192)
  {
    for(J=0;J<SND_CHANNELS;J++)
      if(MasterSwitch&(Switch^MasterSwitch)&(1<<J)) NoteOff(J);
    /*@@@ NO HW VOLUME midiOutSetVolume((UINT)hMIDI,Volume*0x1000100L);*/
  }
}

/** SetWave() ************************************************/
/** Set waveform for a given channel. The channel will be   **/
/** marked with sound type SND_WAVE. Set Rate=0 if you want **/
/** waveform to be an instrument or set it to the waveform  **/
/** own playback rate.                                      **/
/*************************************************************/
void WinSetWave(int Channel,const signed char *Data,int Length,int Rate)
{
  /* Channel has to be valid */
  if((Channel<0)||(Channel>=SND_CHANNELS)) return;
  /* SetWave() call only makes sense for waveOut sound, not midiOut */
  if(SndRate<8192) WinSetSound(Channel,SND_MELODIC);
}

/** NoteOn() *************************************************/
/** Play a new note.                                        **/
/*************************************************************/
void NoteOn(int Channel,int Note,int Level)
{
  Note  = Note<0? 0x00:Note>0x7F? 0x7F:Note;
  Level = Level<0? 0x00:Level>0x7F? 0x7F:Level;

  if(hMIDI&&((Note!=CH[Channel].Note)||(Level!=CH[Channel].Level)))
  {
    if(CH[Channel].Note>=0) NoteOff(Channel);
    midiOutShortMsg
    (
      hMIDI,
      0x90
      | (SHIFT_CHANNEL(Channel)&0x0F)
      | (Level<<16)
      | (Note<<8)
    );
    CH[Channel].Note  = Note;
    CH[Channel].Level = Level;
  }
}

/** NoteOff() ************************************************/
/** Turn off a note.                                        **/
/*************************************************************/
void NoteOff(int Channel)
{
  if(hMIDI&&(CH[Channel].Note>=0))
  {
    midiOutShortMsg
    (
      hMIDI,
      0x80
      | (SHIFT_CHANNEL(Channel)&0x0F)
      | ((CH[Channel].Note&0x7F)<<8)
    );
    CH[Channel].Note=-1;
  }
}
