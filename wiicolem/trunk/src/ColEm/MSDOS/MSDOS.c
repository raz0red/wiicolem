/** ColEm: Portable Coleco Emulator **************************/
/**                                                         **/
/**                        MSDOS.c                          **/
/**                                                         **/
/** This file contains MSDOS-dependent subroutines and      **/
/** drivers. It includes common drivers from Common.h.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Coleco.h"
#include "Console.h"
#include "EMULib.h"
#include "Sound.h"
#include <string.h>

#define WIDTH       272                   /* Buffer width    */
#define HEIGHT      200                   /* Buffer height   */

#define REQ_SNAP    0x01                 /* Make a snapshot  */
#define REQ_RESET   0x02                 /* Reset hardware   */
#define REQ_SAVE    0x04                 /* Save state       */
#define REQ_LOAD    0x08                 /* Load state       */
#define REQ_LOGSND  0x10                 /* Toggle sound log */
#define REQ_MENU    0x20                 /* Invoke menu      */

int Requests;       /* REQ_* values, set by KeyHandler()     */
int InGraphics;     /* 1: VGA is in graphical screen mode    */
int InMenu;         /* 1: In MenuColeco(), ignore keys       */
int UseSound    = 22050;   /* Audio sampling frequency (Hz)  */
int UseEffects  = 0;       /* EFF_* effects to apply         */
int SyncFreq    = -1;      /* Sync frequency (-1=VBlanks)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int Keypad;                /* Keypad key being pressed       */
char *Title     = "ColEm-MSDOS 2.5";      /* Program version */
Image Screen;                             /* Display buffer  */

void HandleKeys(unsigned int Key);

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  int I,J,K;

  /* Initialize variables */
  InGraphics  = 0;
  InMenu      = 0;
  Requests    = 0;
  FastForward = 0;
  Keypad      = 0;

  /* Initialize video */
  if(!NewImage(&Screen,WIDTH,HEIGHT)) return(0);
  SetVideo(&Screen,0,0,WIDTH,HEIGHT);
  ScrWidth  = WIDTH;
  ScrHeight = HEIGHT;
  ScrBuffer = Screen.Data;

  /* Initialize IRQ handlers, etc. */
  InitMSDOS();

  /* Initialize audio */
  InitSound(UseSound,100);

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);
  
  /* Initialize sound */
  SndSwitch=(1<<SN76489_CHANNELS)-1;
  SndVolume=255/SN76489_CHANNELS;
  SetChannels(SndVolume,SndSwitch);

  /* Initialize sync timer if needed, default to VGA VBlanks */
  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=-1;
  /* Use appropriate effects bits */
  UseEffects |= (SyncFreq>0? EFF_SYNC:SyncFreq<0? EFF_VSYNC:0)
             |  (UseEffects&EFF_TVLINES? EFF_SCALE:0);

  /* Go into graphics mode */
  if(UseEffects&(EFF_SCALE|EFF_SOFTEN)) VGA640x480(); else VGA320x200();
  InGraphics=1;

  /* Set initial image effects */
  VGASetEffects(UseEffects);

  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  FreeImage(&Screen);
  TrashAudio();
  TrashMSDOS();
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void *Buffer,int Width,int Height)
{
  /* If in text mode... */
  if(!InGraphics)
  {
    /* Go into graphics mode */
    if(UseEffects&(EFF_SCALE|EFF_SOFTEN)) VGA640x480(); else VGA320x200();
    /* We are in graphics now */
    InGraphics=1;
  }

  /* Show display buffer */
  ShowVideo();
}

/** Mouse() **************************************************/
/** This function is called to poll mouse. It should return **/
/** the X coordinate (-512..+512) in the lower 16 bits, the **/
/** Y coordinate (-512..+512) in the next 14 bits, and the  **/
/** mouse buttons in the upper 2 bits. All values should be **/
/** counted from the screen center!                         **/
/*************************************************************/
unsigned int Mouse(void)
{
  unsigned int X,Y;

  X = GetMouse();
  Y = ((((X>>16)&0x0FFF)<<10)/200-512)&0x3FFF;
  Y = (Y<<16)|((((X&0x0FFF)<<10)/320-512)&0xFFFF);
  return(Y|(X&0xC0000000));
}

/** Joystick() ***********************************************/
/** This function is called to poll joysticks. It should    **/
/** return 2x16bit values in a following way:               **/
/**                                                         **/
/**      x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0      **/
/**                                                         **/
/** Least significant bit on the right. Active value is 1.  **/
/*************************************************************/
unsigned int Joystick(void)
{
  unsigned int J,I;

  /* Render audio here */
  RenderAndPlayAudio(GetFreeAudio());

  if(Requests)
  {
    switch(Requests)
    {
#ifdef GIFLIB
      case REQ_SNAP:   if(InGraphics) SaveGIF(NewFile("SNAP.GIF"));break;
#endif
      case REQ_LOAD:   LoadSTA(StaName? StaName:"DEFAULT.STA");break;
      case REQ_SAVE:   SaveSTA(StaName? StaName:"DEFAULT.STA");break;
      case REQ_LOGSND: MIDILogging(MIDI_TOGGLE);break;
      case REQ_RESET:  ResetColeco(Mode);break;
      case REQ_MENU:   InMenu=1;MenuColeco();InMenu=0;break;
    }

    /* Request handled */
    Requests=0;
  }

  /* Get joystick state */
  I=0;
  J=GetJoystick();
  if(J&BTN_SHIFT)       J|=J&BTN_ALT? (BTN_FIREA<<16):BTN_FIREA;
  if(J&BTN_CONTROL)     J|=J&BTN_ALT? (BTN_FIREB<<16):BTN_FIREB;
  if(J&BTN_LEFT)        I|=JST_LEFT;
  if(J&BTN_RIGHT)       I|=JST_RIGHT;
  if(J&BTN_UP)          I|=JST_UP;
  if(J&BTN_DOWN)        I|=JST_DOWN;
  if(J&BTN_FIREA)       I|=JST_FIRER;
  if(J&BTN_FIREB)       I|=JST_FIREL;
  if(J&BTN_FIREL)       I|=JST_PURPLE;
  if(J&BTN_FIRER)       I|=JST_BLUE;
  if(J&(BTN_LEFT<<16))  I|=JST_LEFT<<16;
  if(J&(BTN_RIGHT<<16)) I|=JST_RIGHT<<16;
  if(J&(BTN_UP<<16))    I|=JST_UP<<16;
  if(J&(BTN_DOWN<<16))  I|=JST_DOWN<<16;
  if(J&(BTN_FIREA<<16)) I|=JST_FIRER<<16;
  if(J&(BTN_FIREB<<16)) I|=JST_FIREL<<16;
  if(J&(BTN_FIREL<<16)) I|=JST_PURPLE<<16;
  if(J&(BTN_FIRER<<16)) I|=JST_BLUE<<16;

  /* Done */
  return(I|(J&BTN_ALT? (Keypad<<16):Keypad));
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
int SetColor(byte N,byte R,byte G,byte B)
{
  return(PIXEL(R,G,B));
//  SetPalette(N+1,R,G,B);
//  return(N+1);
}

/** HandleKeys() *********************************************/
/** Keyboard handler.                                       **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  unsigned int J;

  /* When in MenuColeco() or ConDebug(), ignore keypresses */
  if(InMenu||CPU.Trace) return;

  /* Handle special keys */
  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
      case 0x1C9:
      case CON_F9:
        if(FastForward)
        { VDP.DrawFrames=UPeriod=FastForward;FastForward=0; }
        break;
      case '0':     Keypad&=~JST_0;break;
      case '1':     Keypad&=~JST_1;break;
      case '2':     Keypad&=~JST_2;break;
      case '3':     Keypad&=~JST_3;break;
      case '4':     Keypad&=~JST_4;break;
      case '5':     Keypad&=~JST_5;break;
      case '6':     Keypad&=~JST_6;break;
      case '7':     Keypad&=~JST_7;break;
      case '8':     Keypad&=~JST_8;break;
      case '9':     Keypad&=~JST_9;break;
      case '-':     Keypad&=~JST_STAR;break;
      case '=':     Keypad&=~JST_POUND;break;
    }
  else
    switch(Key&CON_KEYCODE)
    {
#ifdef DEBUG
      case CON_F1:  CPU.Trace=1;break;
#endif
      case CON_F2:  Requests|=REQ_LOGSND;break;
      case CON_F3:  Mode^=CV_AUTOFIRER;break;
      case CON_F4:  Mode^=CV_AUTOFIREL;break;
      case CON_F5:  Requests|=REQ_MENU;break;
      case CON_F6:  Requests|=REQ_LOAD;break;
      case CON_F7:  Requests|=REQ_SAVE;break;
      case CON_F8:
        UseEffects^=Key&CON_ALT? EFF_SOFTEN:(EFF_TVLINES|EFF_SCALE);
        VGASetEffects(UseEffects);
        /* Make sure screen gets changed/cleared */
        InGraphics=0;
        break;
      case 0x1C9:
      case CON_F9:
        if(!FastForward)
        { FastForward=UPeriod;VDP.DrawFrames=UPeriod=10; }
        break;
      case CON_F10: Requests|=REQ_SNAP;break;
      case CON_F11: Requests|=REQ_RESET;break;
      case 0x1B:
      case CON_F12: ExitNow=1;break;
      case '0':     Keypad|=JST_0;break;
      case '1':     Keypad|=JST_1;break;
      case '2':     Keypad|=JST_2;break;
      case '3':     Keypad|=JST_3;break;
      case '4':     Keypad|=JST_4;break;
      case '5':     Keypad|=JST_5;break;
      case '6':     Keypad|=JST_6;break;
      case '7':     Keypad|=JST_7;break;
      case '8':     Keypad|=JST_8;break;
      case '9':     Keypad|=JST_9;break;
      case '-':     Keypad|=JST_STAR;break;
      case '=':     Keypad|=JST_POUND;break;
    }
}
