/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                        Maemo.c                          **/
/**                                                         **/
/** This file contains Maemo-dependent subroutines and      **/
/** drivers.                                                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Coleco.h"
#include "Console.h"
#include "EMULib.h"
#include "NetPlay.h"
#include "Sound.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DBUS_SERVICE "com.fms.colem"

#define WIDTH       272                   /* Buffer width    */
#define HEIGHT      200                   /* Buffer height   */

#define MENU_OPEN    0x1001
#define MENU_FREEZE  0x1002
#define MENU_RESTORE 0x1003
#define MENU_CONFIG  0x1004
#define MENU_DEBUG   0x1005
#define MENU_RESET   0x1006
#define MENU_ABOUT   0x1007
#define MENU_TVLINES 0x1008
#define MENU_SOFTEN  0x1009
#define MENU_PENCUES 0x100A
#define MENU_SHOWFPS 0x100B

/* Combination of EFF_* bits */
int UseEffects  = EFF_SCALE|EFF_SAVECPU|EFF_SYNC|EFF_DIALCUES;

int InMenu;                /* 1: In MenuColeco(),ignore keys */
int UseSound    = 44100;   /* Audio sampling frequency (Hz)  */
int SyncFreq    = 60;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int Keypad;                /* Keypad key being pressed       */
char FileToOpen[256];      /* Filename to open, from DBus    */
Image ScrBuf;              /* Display buffer                 */

const char *Title = "ColEm Maemo 2.5";    /* Program version */

void HandleKeys(unsigned int Key);
int HandleFiles(const char *Filename);

/** ColEm.c **************************************************/
/** The normal, command-line main() is taken from here.     **/
/*************************************************************/
#define main MyMain
#include "ColEm.c"
#undef main

/** main() ***************************************************/
/** Execution starts HERE and not in the ColEm.c main()!    **/
/*************************************************************/
int main(int argc,char *argv[])
{
  char S[sizeof(FileToOpen)];
  const char *P;

  /* This is where BIOS files are in Maemo installation */
  HomeDir = "/usr/share/colem";
  UPeriod = 35;
  ARGC    = argc;
  ARGV    = argv;

#ifdef DEBUG
  CPU.Trap  = 0xFFFF;
  CPU.Trace = 0;
#endif

  if(argc>1) return(MyMain(argc,argv));
  else
  {
    /* Initialize hardware */
    if(!InitMachine()) return(1);
    /* If no filename passed over DBus, ask for a file name */
    P = FileToOpen[0]? strcpy(S,FileToOpen):GTKAskFilename(GTK_FILE_CHOOSER_ACTION_OPEN);
    FileToOpen[0]='\0';
    if(!P) return(1);
    /* Run emulation, then clean up */
    StartColeco(P);
    TrashColeco();
    /* Release hardware */
    TrashMachine();
  }

  /* Done */
  return(0);
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  /* Initialize variables */
  InMenu         = 0;
  FastForward    = 0;
  ScrBuf.Data    = 0;
  FileToOpen[0]  = '\0';
  Keypad         = 0;

  /* Set visual effects */
  GTKSetEffects(UseEffects);

  /* Initialize system resources */
  InitMaemo(Title,WIDTH*2,HEIGHT*2,DBUS_SERVICE);

  /* Create main image buffer */
  if(!NewImage(&ScrBuf,WIDTH,HEIGHT)) return(0);
  SetVideo(&ScrBuf,0,0,WIDTH,HEIGHT);
  ScrWidth  = WIDTH;
  ScrHeight = HEIGHT;
  ScrBuffer = ScrBuf.Data;

  /* Attach file handler */
  SetFileHandler(HandleFiles);
  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);

  /* Add menus */
  GTKAddMenu("Open File",MENU_OPEN);
  GTKAddMenu("Freeze State",MENU_FREEZE);
  GTKAddMenu("Restore State",MENU_RESTORE);
  GTKAddMenu(0,0);
  GTKAddMenu("Configuration",MENU_CONFIG);
  GTKAddMenu("Debugger",MENU_DEBUG);
  GTKAddMenu(0,0);
  GTKAddMenu("Scanlines",MENU_TVLINES);
  GTKAddMenu("Interpolation",MENU_SOFTEN);
  GTKAddMenu("Stylus Cues",MENU_PENCUES);
  GTKAddMenu("Framerate",MENU_SHOWFPS);
  GTKAddMenu(0,0);
  GTKAddMenu("About",MENU_ABOUT);
  GTKAddMenu("Reset",MENU_RESET);

  /* Read and activate settings */
  UPeriod    = GTKGetInteger("draw-frames",UPeriod);
  Mode       = GTKGetInteger("hardware-mode",Mode);
  UseEffects = GTKGetInteger("video-effects",UseEffects);
  GTKSetEffects(UseEffects);

  /* Initialize sound */
  InitSound(UseSound,150);
  SndSwitch=(1<<SN76489_CHANNELS)-1;
  SndVolume=255/SN76489_CHANNELS;
  SetChannels(SndVolume,SndSwitch);

  /* Initialize sync timer if needed */
  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=0;

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  /* Cancel fast-forwarding */
  if(FastForward)
  {
    GTKSetEffects(UseEffects);
    VDP.DrawFrames=UPeriod=FastForward;
    FastForward=0;
  }

  /* Save settings */
  GTKSetInteger("draw-frames",UPeriod);
  GTKSetInteger("hardware-mode",Mode);
  GTKSetInteger("video-effects",UseEffects);

  /* Free resources */
  FreeImage(&ScrBuf);
  TrashAudio();
  TrashMaemo();
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void *Buffer,int Width,int Height)
{
  /* Display the screen buffer */
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
  unsigned int J,I,K;

  /* If somebody asked us to open a file, load it */
  if(FileToOpen[0])
  {
    if(!LoadROM(FileToOpen)) GTKMessage("Failed loading file.");
    FileToOpen[0]='\0';
  }

  /* Render audio (two frames to avoid skipping) */
  RenderAndPlayAudio(UseSound/30);

  /* Get joystick state */
  I = 0;
  J = GetJoystick()|PenJoystick();
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

  /* Get on-screen keypad state */
  K = Keypad;
  switch(PenDialpad())
  {
    case DIAL_0:     K|=JST_0;break;
    case DIAL_1:     K|=JST_1;break;
    case DIAL_2:     K|=JST_2;break;
    case DIAL_3:     K|=JST_3;break;
    case DIAL_4:     K|=JST_4;break;
    case DIAL_5:     K|=JST_5;break;
    case DIAL_6:     K|=JST_6;break;
    case DIAL_7:     K|=JST_7;break;
    case DIAL_8:     K|=JST_8;break;
    case DIAL_9:     K|=JST_9;break;
    case DIAL_STAR:  K|=JST_STAR;break;
    case DIAL_POUND: K|=JST_POUND;break;
  }

  /* Add keypad buttons */
  I|=J&BTN_ALT? (K<<16):K;

  /* If exchanged keymaps with remote... */
  if(NETExchange((char *)&J,(const char *)&I,sizeof(I)))
    /* Merge joysticks, server is player #1, client is player #2 */
    I = NETConnected()==NET_SERVER?
        ((I&0xFFFF)|((J&0xFFFF)<<16))
      : ((J&0xFFFF)|((I&0xFFFF)<<16));

  /* Done */
  return(I);
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
int SetColor(byte N,byte R,byte G,byte B)
{
  return(PIXEL(R,G,B));
}

/** HandleFiles() ********************************************/
/** Handle open-file messages coming from DBus.             **/
/*************************************************************/
int HandleFiles(const char *Filename)
{
  /* Make a request to open file */
  if(FileToOpen[0]||(strlen(Filename)>=sizeof(FileToOpen))) return(0);
  strcpy(FileToOpen,Filename);
  return(1);
}

/** HandleKeys() *********************************************/
/** Keyboard handler.                                       **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  const char *P;

  /* When F12 pressed, exit no matter what */
  if((Key&CON_KEYCODE)==GDK_F12) { ExitNow=1;return; }

  /* When in MenuColeco() or ConDebug(), ignore keypresses */
  if(InMenu||CPU.Trace) return;

  /* Handle special keys */
  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
      case GDK_F4:
        if(FastForward)
        {
          GTKSetEffects(UseEffects);
          VDP.DrawFrames=UPeriod=FastForward;
          FastForward=0;
        }
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
      case MENU_DEBUG:    CPU.Trace=1;break;
#endif
      case MENU_RESTORE:  LoadSTA(StaName? StaName:"DEFAULT.STA");break;
      case MENU_FREEZE:   SaveSTA(StaName? StaName:"DEFAULT.STA");break;
      case MENU_CONFIG:   InMenu=1;MenuColeco();InMenu=0;break;
      case MENU_RESET:    ResetColeco(Mode);break;
      case MENU_SOFTEN:   GTKSetEffects(UseEffects^=EFF_SOFTEN);break;
      case MENU_TVLINES:  GTKSetEffects(UseEffects^=EFF_TVLINES);break;
      case MENU_PENCUES:  GTKSetEffects(UseEffects^=EFF_PENCUES);break;
      case MENU_SHOWFPS:  GTKSetEffects(UseEffects^=EFF_SHOWFPS);break;

      case MENU_OPEN:
        P=GTKAskFilename(GTK_FILE_CHOOSER_ACTION_OPEN);
        if(P&&!LoadSTA(P)&&!LoadROM(P)) GTKMessage("Failed loading file.");
        break;

      case MENU_ABOUT:
        GTKMessage(
          "ColEm-Maemo by Marat Fayzullin\n"
          "ColecoVision emulator\n"
          "compiled on "__DATE__"\n"
          "\n"
          "Please, visit http://fms.komkon.org/\n"
          "for news, updates, and documentation"
        );
        break;

      case GDK_F4:
        if(!FastForward)
        {
          GTKSetEffects(UseEffects&~EFF_SYNC);
          FastForward=UPeriod;
          VDP.DrawFrames=UPeriod=10;
        }
        break;

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
