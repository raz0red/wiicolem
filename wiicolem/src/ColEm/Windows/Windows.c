/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                        Windows.c                        **/
/**                                                         **/
/** This file contains the Windows-specific code for the    **/
/** ColEm emulator. It uses EMULib-Windows libraries.       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Coleco.h"
#include "EMULib.h"
#include "NetPlay.h"
#include "Console.h"
#include "Sound.h"
#include "ColEm.rh"

#define WIDTH       272                   /* Buffer width    */
#define HEIGHT      200                   /* Buffer height   */

const char *AppTitle = "ColEm 2.5";
const char *AppClass = "ColEm";
const char *Title    = "ColEm-Windows 2.5";
int AppWidth  = WIDTH*2;
int AppHeight = HEIGHT*2;
int AppMode   = MODE_640x480;

Image Screen;              /* Main screen image              */
int UseSound    = 22050;   /* Audio sampling frequency (Hz)  */
int SwapButtons = 0;       /* 1: Swap FIREL/FIRER buttons    */
int VideoType   = 50;      /* 50: PAL, 60: NTSC, 0: no-sync  */    
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int InHandleKeys;          /* 1: Locked inside HandleKeys()  */
int Keypad;                /* Pressed keypad button          */

static const char *FileTitle =
  "Open Coleco File";
static const char *FileExts =
  "Supported Files (*.rom,*.cv,*.rom.gz,*.cv.gz)\0*.rom;*.cv;*.rom.gz;*.cv.gz\0"
  "All Files (*.*)\0*.*\0";

/** ColEm.c **************************************************/
/** main() function common for all ports.                   **/
/*************************************************************/
#define main MyMain
#include "ColEm.c"

void SetMenus(void)
{
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_JoySwap,SwapButtons? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_AllSprites,Mode&CV_ALLSPRITE? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_AutoA,Mode&CV_AUTOFIRER? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_AutoB,Mode&CV_AUTOFIREL? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_Spin1X,Mode&CV_SPINNER1X? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_Spin1Y,Mode&CV_SPINNER1Y? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_Spin2X,Mode&CV_SPINNER2X? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_Spin2Y,Mode&CV_SPINNER2Y? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_Drums,Mode&CV_DRUMS? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),WMENU_SNDLOG,MIDILogging(MIDI_QUERY)? MF_CHECKED:MF_UNCHECKED);

  CheckMenuRadioItem(
    GetMenu(WinGetAppWindow()),
    CM_Adam,CM_CVision,
    Mode&CV_ADAM? CM_Adam:CM_CVision,
    MF_BYCOMMAND
  );

  CheckMenuRadioItem(
    GetMenu(WinGetAppWindow()),
    CM_PAL,CM_NTSC,
    Mode&CV_PAL? CM_PAL:CM_NTSC,
    MF_BYCOMMAND
  );

  CheckMenuRadioItem(
    GetMenu(WinGetAppWindow()),
    CM_Palette0,CM_Palette2,
    (Mode&CV_PALETTE)==CV_PALETTE2? CM_Palette2 :
    (Mode&CV_PALETTE)==CV_PALETTE1? CM_Palette1 : CM_Palette0,
    MF_BYCOMMAND
  );

// @@@ HAS TO BE SET AFTER StartColeco()!!!
//  EnableMenuItem(GetMenu(WinGetAppWindow()),CM_Adam,AdamROMs? MF_ENABLED:MF_GRAYED);  

  // Currently disabling Adam option
  EnableMenuItem(GetMenu(WinGetAppWindow()),CM_Adam,MF_GRAYED);  
}

void SetVideoSync(int NewVideoType)
{
  VideoType=NewVideoType;
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_PALVideo,VideoType==50? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(WinGetAppWindow()),CM_NTSCVideo,VideoType==60? MF_CHECKED:MF_UNCHECKED);
  /* Adjust sync frequency */
  SetSyncTimer(VideoType*UPeriod/100);
}

void SetUPeriod(int NewUPeriod)
{
  static const int UPeriods[] = { 35,50,75,100,-1 };
  int J;

  /* fast forwarding off, new UPeriod */
  VDP.DrawFrames=UPeriod=NewUPeriod;
  FastForward=0;

  /* Adjust sync frequency */
  SetSyncTimer(VideoType*UPeriod/100);

  /* Find and check menu option */
  for(J=0;(UPeriods[J]>0)&&(UPeriods[J]!=UPeriod);++J);
  if(UPeriods[J]>0)
    CheckMenuRadioItem(
      GetMenu(WinGetAppWindow()),
      CM_Draw35,CM_Draw100,
      CM_Draw35+J,
      MF_BYCOMMAND
    );
}

/** Application() ********************************************/
/** Application entry point.                                **/
/*************************************************************/
int Application(int argc,char *argv[])
{
  if(argc>1) return(MyMain(argc,argv));
  else
  {
    char *args[2];

    args[0]=argv[0];
    args[1]=(char *)WinAskFilename(FileTitle,FileExts,ASK_ASSOCIATE);
    return(MyMain(args[1]? 2:1,args));
  }
}

/** HandleCmds() *********************************************/
/** Command handler.                                        **/
/*************************************************************/
void HandleCmds(unsigned int Cmd)
{
  static const int UPeriods[] = { 35,50,75,100 };
  const char *P;

  switch(Cmd)
  {
    case CM_Open:
      P=WinAskFilename(FileTitle,FileExts,ASK_ASSOCIATE);
      if(P&&!LoadROM(P)) MessageBox(WinGetAppWindow(),"Failed loading file.","ColEm",MB_ICONERROR);
      break;
    case CM_SaveSTA:
      P=WinAskFilename(
        "Save .STA Snapshot",
        "State Snapshots (*.sta,*.sta.gz)\0*.sta;*.sta.gz\0All Files (*.*)\0*.*\0",
        ASK_SAVEFILE
      );
      if(P&&!SaveSTA(P)) MessageBox(WinGetAppWindow(),"Failed saving state.","ColEm",MB_ICONERROR);
      break;
    case CM_LoadSTA:
      P=WinAskFilename(
        "Load .STA Snapshot",
        "State Snapshots (*.sta,*.sta.gz)\0*.sta;*.sta.gz\0All Files (*.*)\0*.*\0",
        0
      );
      if(P&&!LoadSTA(P)) MessageBox(WinGetAppWindow(),"Failed loading state.","ColEm",MB_ICONERROR);
      break;
    case CM_About:
      MessageBox(
        WinGetAppWindow(),
        "COLEM\n"
        "compiled on "__DATE__"\n\n"
        "Coleco Emulator\n"
        "by Marat Fayzullin\n\n"
        "http://fms.komkon.org/",
        "About ColEm",
        MB_ICONINFORMATION
      );
      break;
    case CM_JoySwap:
      SwapButtons=!SwapButtons;
      SetMenus();
      break;
    case CM_PALVideo:
      SetVideoSync(GetMenuState(GetMenu(WinGetAppWindow()),CM_PALVideo,MF_BYCOMMAND)&MF_CHECKED? 0:50);
      break;
    case CM_NTSCVideo:
      SetVideoSync(GetMenuState(GetMenu(WinGetAppWindow()),CM_NTSCVideo,MF_BYCOMMAND)&MF_CHECKED? 0:60);
      break;
    case CM_Reset:
      ResetColeco(Mode);
      break;
    case CM_Draw35:
    case CM_Draw50:
    case CM_Draw75:
    case CM_Draw100:
      SetUPeriod(UPeriods[Cmd-CM_Draw35]);
      break;
    case CM_AutoA:
      Mode^=CV_AUTOFIRER;
      SetMenus();
      break;
    case CM_AutoB:
      Mode^=CV_AUTOFIREL;
      SetMenus();
      break;
    case CM_Spin1X:
      Mode^=CV_SPINNER1X;
      if(Mode&CV_SPINNER1X) Mode&=~CV_SPINNER1Y;
      SetMenus();
      break;
    case CM_Spin1Y:
      Mode^=CV_SPINNER1Y;
      if(Mode&CV_SPINNER1Y) Mode&=~CV_SPINNER1X;
      SetMenus();
      break;
    case CM_Spin2X:
      Mode^=CV_SPINNER2X;
      if(Mode&CV_SPINNER2X) Mode&=~CV_SPINNER2Y;
      SetMenus();
      break;
    case CM_Spin2Y:
      Mode^=CV_SPINNER2Y;
      if(Mode&CV_SPINNER2Y) Mode&=~CV_SPINNER2X;
      SetMenus();
      break;
    case CM_Drums:
      Mode^=CV_DRUMS;
      SetMenus();
      break;
    case CM_Adam:
      ResetColeco(Mode|CV_ADAM);
      SetMenus();
      break;
    case CM_CVision:
      ResetColeco(Mode&~CV_ADAM);
      SetMenus();
      break;
    case CM_PAL:
      ResetColeco(Mode|CV_PAL);
      SetVideoSync(50);
      SetMenus();
      break;
    case CM_NTSC:
      ResetColeco(Mode&~CV_PAL);
      SetVideoSync(60);
      SetMenus();
      break;
    case CM_AllSprites:
      Mode^=CV_ALLSPRITE;
      VDP.MaxSprites=Mode&CV_ALLSPRITE? 255:TMS9918_MAXSPRITES; 
      SetMenus();
      break;
    case CM_Palette0:
      ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE0);
      SetMenus();
      break;
    case CM_Palette1:
      ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE1);
      SetMenus();
      break;
    case CM_Palette2:
      ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE2);
      SetMenus();
      break;
    case CM_Joysticks:
      WinSetJoysticks(BTN_FIREA|BTN_FIREB|BTN_FIREL|BTN_FIRER);
      break;
    case WMENU_NETPLAY:
      /* Reset Coleco when NetPlay connected/disconnected */
      ResetColeco(Mode);
      break;
    case WMENU_QUIT:
      ExitNow=1;
      break;
  }
}

/** HandleKeys() *********************************************/
/** Key handler.                                            **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  const char *P;

  if(InHandleKeys||CPU.Trace) return; else InHandleKeys=1;

  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
      case VK_F9:
      case VK_PRIOR:
        if(FastForward) { VDP.DrawFrames=UPeriod=FastForward;FastForward=0; }
        break;
      case '0':        Keypad&=~JST_0;break;
      case '1':        Keypad&=~JST_1;break;
      case '2':        Keypad&=~JST_2;break;
      case '3':        Keypad&=~JST_3;break;
      case '4':        Keypad&=~JST_4;break;
      case '5':        Keypad&=~JST_5;break;
      case '6':        Keypad&=~JST_6;break;
      case '7':        Keypad&=~JST_7;break;
      case '8':        Keypad&=~JST_8;break;
      case '9':        Keypad&=~JST_9;break;
      case VK_OEM_MINUS:
                       Keypad&=~JST_STAR;
                       break;
      case VK_OEM_PLUS:
                       Keypad&=~JST_POUND;
                       break;
    }
  else
    switch(Key&CON_KEYCODE)
    {
#ifdef DEBUG
      case VK_F1:      CPU.Trace=1;break;
#endif
      case VK_F2:      MIDILogging(MIDI_TOGGLE);SetMenus();break;
      case VK_F3:      Mode^=CV_AUTOFIRER;SetMenus();break;
      case VK_F4:      Mode^=CV_AUTOFIREL;SetMenus();break;
      case VK_F5:
        SetChannels(0,0);
        MenuColeco();
        SetChannels(SndVolume,SndSwitch);
        SetMenus();
        break;
      case VK_F6:      LoadSTA(StaName? StaName:"DEFAULT.STA");break;
      case VK_F7:      SaveSTA(StaName? StaName:"DEFAULT.STA");break;
      case VK_F9:
      case VK_PRIOR:
        if(!FastForward) { FastForward=UPeriod;VDP.DrawFrames=UPeriod=10; }
        break;
      case VK_F11:     ResetColeco(Mode);break;
      case VK_F12:
      case VK_ESCAPE:  DestroyWindow(WinGetAppWindow());break;
      case '0':        Keypad|=JST_0;break;
      case '1':        Keypad|=JST_1;break;
      case '2':        Keypad|=JST_2;break;
      case '3':        Keypad|=JST_3;break;
      case '4':        Keypad|=JST_4;break;
      case '5':        Keypad|=JST_5;break;
      case '6':        Keypad|=JST_6;break;
      case '7':        Keypad|=JST_7;break;
      case '8':        Keypad|=JST_8;break;
      case '9':        Keypad|=JST_9;break;
      case VK_OEM_MINUS:
                       Keypad|=JST_STAR;
                       break;
      case VK_OEM_PLUS:
                       Keypad|=JST_POUND;
                       break;
    }

  InHandleKeys=0;
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  /* Initialize video */
  if(!NewImage(&Screen,WIDTH,HEIGHT)) return(0);
  SetVideo(&Screen,0,0,WIDTH,HEIGHT);
  ScrWidth  = WIDTH;
  ScrHeight = HEIGHT;
  ScrBuffer = Screen.Data;

  /* Initialize variables */
  InHandleKeys = 0;
  FastForward  = 0;
  Keypad       = 0;

  /* Read settings */
  SwapButtons = WinGetInteger(0,"SwapButtons",SwapButtons);
  VideoType   = WinGetInteger(0,"VideoType",VideoType);
  UseSound    = WinGetInteger(0,"UseSound",UseSound);
  UPeriod     = WinGetInteger(0,"UPeriod",UPeriod);
  Mode        = WinGetInteger(0,"Mode",Mode);

  /* Check menus */
  SetUPeriod(UPeriod);
  SetVideoSync(VideoType);
  SetMenus();

  /* Attach keyboard and command handlers */
  SetKeyHandler(HandleKeys);
  WinSetCmdHandler(HandleCmds);
  
  /* Initialize sound */
  SndSwitch=(1<<SN76489_CHANNELS)-1;
  SndVolume=255/SN76489_CHANNELS;
  SetChannels(SndVolume,SndSwitch);

  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  /* Cancel fastforward */
  if(FastForward) { UPeriod=FastForward;FastForward=0; }

  /* Write settings */
  WinSetInteger(0,"SwapButtons",SwapButtons);
  WinSetInteger(0,"VideoType",VideoType);
  WinSetInteger(0,"UseSound",UseSound);
  WinSetInteger(0,"UPeriod",UPeriod);
  WinSetInteger(0,"Mode",Mode);

  /* Free stuff */
  SetSyncTimer(0);
  FreeImage(&Screen);
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

  /* Rendering audio here */
  RenderAndPlayAudio(GetFreeAudio());

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

  /* Add keypad buttons */
  I|=J&BTN_ALT? (Keypad<<16):Keypad;

  /* Swap fire buttons if needed */
  if(SwapButtons)
    I = (I&~(JST_FIREL|JST_FIRER|(JST_FIREL<<16)|(JST_FIRER<<16)))
      | (I&(JST_FIREL<<16)? (JST_FIRER<<16):0)
      | (I&(JST_FIRER<<16)? (JST_FIREL<<16):0)
      | (I&JST_FIREL? JST_FIRER:0)
      | (I&JST_FIRER? JST_FIREL:0);

  /* If exchanged keymaps with remote... */
  if(NETExchange((char *)&J,(const char *)&I,sizeof(I)))
    /* Merge joysticks, server is player #1, client is player #2 */
    I = NETConnected()==NET_SERVER?
        ((I&0xFFFF)|((J&0xFFFF)<<16))
      : ((J&0xFFFF)|((I&0xFFFF)<<16));

  /* Done */
  return(I);
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
  Y = ((((X>>16)&0x0FFF)<<10)/AppHeight-512)&0x3FFF;
  Y = (Y<<16)|((((X&0x0FFF)<<10)/AppWidth-512)&0xFFFF);
  if(SwapButtons) X=((X&0x80000000)>>1)|((X&0x40000000)<<1);
  return(Y|(X&0xC0000000));
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void *Buffer,int Width,int Height)
{
  /* Synchronize to the timer */
  if(!FastForward) WaitSyncTimer();
  /* Display the screen buffer */
  ShowVideo();
}

/** SetColor() ***********************************************/
/** Set color N (0..15) to (R,G,B).                         **/
/*************************************************************/
int SetColor(byte N,byte R,byte G,byte B)
{
  return(PIXEL(R,G,B));
}
