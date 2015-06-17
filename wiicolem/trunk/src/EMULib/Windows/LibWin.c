/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibWin.c                         **/
/**                                                         **/
/** This file contains Windows-dependent implementation     **/
/** parts of the emulation library.                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#define INITGUID

#include <WinSock2.h>
#include <Windows.h>
#include <DDraw.h>
#include <stdio.h>

#include "EMULib.h"
#include "NetPlay.h"
#include "Sound.h"

/** Video Modes **********************************************/
/** These can be ORed for special effects in ShowVideo().   **/
/*************************************************************/
#define EFF_NONE   0x0000
#define EFF_SYNC   0x0001
#define EFF_TV     0x0002
#define EFF_SOFTEN 0x0004

static HINSTANCE hInst;           /* Application instance    */
static HWND hWnd;                 /* Main window             */
int AppX;                         /* Main window coordinates */
int AppY;

static int TimerID;               /* Sync timer ID or 0      */
static int TimerReady;            /* 1: sync timer is ready  */
static unsigned int JoyState;     /* Result of GetJoystick() */
static unsigned int MousePos;     /* Result of GetMouse()    */
static unsigned int LastKey;      /* Result of GetKey()      */
static unsigned int KeyModes;     /* CON_SHIFT/CONTROL/ALT   */
static unsigned int FullScreen;   /* 1: Running full-screen  */
static unsigned int Effects;      /* ORed EFF_* bits         */
static unsigned int FocusOut;     /* 1: Window out of focus  */
static unsigned int SaveCPU;      /* 1: Freeze on FocusOut=1 */
static char OpenFilename[256];    /* Result of AskFilename() */
static char SaveFilename[256];    /* Result of AskFilename() */
static char Fullpath[256];        /* Executable path+name    */
static char MIDIName[256];        /* MIDI sountrack name     */
Image BigScreen;                  /* Video interp-tion buf.  */

extern int MasterVolume;          /* Imported from Sound.c   */
extern int MasterSwitch;          /* Imported from Sound.c   */
static int BackupSwitch;          /* Used to backup MasterSw */
static int ISDelay;               /* InitSound() delay (ms)  */
static int ISRate;                /* InitSound() rate (Hz)   */

static HWAVEOUT hWave = 0;        /* WaveOut device handle   */
static DWORD ThreadID = 0;        /* Audio thread ID         */
static int SndSize    = 0;        /* SndBuffer[] size        */
static int SndRate    = 0;        /* Sound playback rate     */
static int CurHdr;                /* Next header to output   */
static WAVEHDR SndHeader[SND_HEADERS]; /* WaveOut headers    */
static sample *SndBuffer = 0;     /* Ring buffer for audio   */
static sample *SndWPtr,*SndRPtr;  /* Read and write pointers */

static int JoyButtons[2][7] =
{
  { 0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040 },
  { 0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040 }
};
static JOYCAPS JoyCaps[2];
static int JoyCount;

static void (*CmdHandler)(unsigned int Cmd);

static LPDIRECTDRAW7 DD                = 0;
static LPDIRECTDRAWCLIPPER DDClipper   = 0;
static LPDIRECTDRAWSURFACE7 DDSurface  = 0;
static HWND DDWindow                   = 0;
static int DDFullScreen                = 0;
static HMENU DDMenu;
static RECT DDRect;

static LRESULT CALLBACK WndProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
  /* Handles main window events. */
static DWORD WINAPI ThreadFunc(void *Param);
  /* Thread used to update audio. */
static int HandleKeys(int Key,int Down);
  /* Handles keyboard events, update joystick state. */
static void PlayBuffers(void);
  /* Streams all filled out audio blocks to the WaveOut device. */
static void CALLBACK TimerProc(UINT TimerID,UINT Msg,DWORD dwUser,DWORD dw1,DWORD dw2);
  /* Sets TimerReady when sync timer expires. */
static int DDInit(HWND W,int Width,int Height);
  /* Initializes DirectDraw. */
static void DDTrash();
  /* Shuts down DirectDraw. */
static int PutImageWH(Image *Img,int X,int Y,int W,int H,int DW,int DH);
  /* Shows image in a window, resizes it to DWxDH. */
static void SetSoundSettings(int Rate,int Delay);
  /* Sets sound driver parameters and corresponding menus */

/** WinGetInteger() ******************************************/
/** Get integer setting by name or return Default if not    **/
/** found.                                                  **/
/*************************************************************/
unsigned int WinGetInteger(const char *Section,const char *Name,unsigned int Default)
{
  char Path[256];
  unsigned long Data,Size,Type;
  HKEY Key;

  Data=Default;
  Size=sizeof(Data);
  Type=REG_DWORD;
  sprintf(Path,"Software\\EMUL8\\%s",AppClass);
  if(Section) { strcat(Path,"\\");strcat(Path,Section); }

  if(!RegOpenKeyEx(HKEY_CURRENT_USER,Path,0,KEY_QUERY_VALUE,&Key))
  {
    RegQueryValueEx(Key,Name,0,&Type,(void *)&Data,&Size);
    RegCloseKey(Key);
  }

  return(Type==REG_DWORD? Data:Default);
}

/** WinSetInteger() ******************************************/
/** Set integer setting to a given value.                   **/
/*************************************************************/
void WinSetInteger(const char *Section,const char *Name,unsigned int Value)
{
  char Path[256];
  unsigned long Data;
  HKEY Key;

  sprintf(Path,"Software\\EMUL8\\%s",AppClass);
  if(Section) { strcat(Path,"\\");strcat(Path,Section); }
  if(!RegCreateKeyEx(HKEY_CURRENT_USER,Path,0,0,0,KEY_SET_VALUE,0,&Key,&Data))
  {
    Data=Value;
    RegSetValueEx(Key,Name,0,REG_DWORD,(void *)&Data,sizeof(Data));
    RegCloseKey(Key);
  }
}

/** WinGetString() *******************************************/
/** Get string setting by name.                             **/
/*************************************************************/
char *WinGetString(const char *Section,const char *Name,char *Value,int MaxChars)
{
  char Path[256];
  unsigned long Size,Type;
  HKEY Key;

  Size=MaxChars;
  Type=REG_SZ;
  sprintf(Path,"Software\\EMUL8\\%s",AppClass);
  if(Section) { strcat(Path,"\\");strcat(Path,Section); }

  if(!RegOpenKeyEx(HKEY_CURRENT_USER,Path,0,KEY_QUERY_VALUE,&Key))
  {
    RegQueryValueEx(Key,Name,0,&Type,(void *)Value,&Size);
    RegCloseKey(Key);
  }

  return(Type==REG_SZ? Value:0);
}

/** WinSetString() *******************************************/
/** Set string setting to a given value.                    **/
/*************************************************************/
void WinSetString(const char *Section,const char *Name,const char *Value)
{
  char Path[256];
  unsigned long Data;
  HKEY Key;

  sprintf(Path,"Software\\EMUL8\\%s",AppClass);
  if(Section) { strcat(Path,"\\");strcat(Path,Section); }
  if(!RegCreateKeyEx(HKEY_CURRENT_USER,Path,0,0,0,KEY_SET_VALUE,0,&Key,&Data))
  {
    RegSetValueEx(Key,Name,0,REG_DWORD,(void *)Value,strlen(Value)+1);
    RegCloseKey(Key);
  }
}

/** WinAssociate() *******************************************/
/** Associates files with given extension to the application */
/*************************************************************/
int WinAssociate(const char *Ext,const char *Comment,int Icon)
{
  char S[256],T[250];
  const char *P;
  unsigned long J;
  HKEY Key;

  /* Create application entry */
  sprintf(S,"%s.File",AppClass);
  if(RegCreateKeyEx(HKEY_CLASSES_ROOT,S,0,0,0,KEY_SET_VALUE,0,&Key,&J))
    return(0);
  else
  {
    sprintf(S,"%s File",AppClass);
    P=Comment? Comment:S;
    RegSetValueEx(Key,0,0,REG_SZ,(void *)P,strlen(P)+1);
    RegCloseKey(Key);
  }

  /* Add extension entry */
  if(RegCreateKeyEx(HKEY_CLASSES_ROOT,Ext,0,0,0,KEY_SET_VALUE,0,&Key,&J))
    return(0);
  else
  {
    sprintf(S,"%s.File",AppClass);
    RegSetValueEx(Key,0,0,REG_SZ,(void *)S,strlen(S)+1);
    RegCloseKey(Key);
  }

  /* If we have got executable name... */
  if(GetModuleFileName(0,T,sizeof(T)))
  {
    /* Add default icon entry */
    sprintf(S,"%s.File\\DefaultIcon",AppClass);
    if(!RegCreateKeyEx(HKEY_CLASSES_ROOT,S,0,0,0,KEY_SET_VALUE,0,&Key,&J))
    {
      sprintf(S,"%s,%d",T,Icon);
      RegSetValueEx(Key,0,0,REG_SZ,(void *)S,strlen(S)+1);
      RegCloseKey(Key);
    }

    /* Add default action */
    sprintf(S,"%s.File\\shell\\open",AppClass);
    if(!RegCreateKeyEx(HKEY_CLASSES_ROOT,S,0,0,0,KEY_SET_VALUE,0,&Key,&J))
    {
      sprintf(S,"Open with %s",AppClass);
      RegSetValueEx(Key,0,0,REG_SZ,(void *)S,strlen(S)+1);
      RegCloseKey(Key);
    }

    /* Add default action command */
    sprintf(S,"%s.File\\shell\\open\\command",AppClass);
    if(!RegCreateKeyEx(HKEY_CLASSES_ROOT,S,0,0,0,KEY_SET_VALUE,0,&Key,&J))
    {
      sprintf(S,"\"%s\" \"%%1\"",T);
      RegSetValueEx(Key,0,0,REG_SZ,(void *)S,strlen(S)+1);
      RegCloseKey(Key);
    }
  }

  /* Done */
  return(1);
}

/** WinGetAppInstance() **************************************/
/** Get application instance.                               **/
/*************************************************************/
HINSTANCE WinGetAppInstance(void) { return(hInst); }

/** WinGetAppWindow() ****************************************/
/** Get main window handle.                                 **/
/*************************************************************/
HWND WinGetAppWindow(void) { return(hWnd); }

/** WinSetCmdHandler() ***************************************/
/** Attach a handler that will be called on menu selections **/
/** and other commands.                                     **/
/*************************************************************/
void WinSetCmdHandler(void (*Handler)(unsigned int Cmd))
{ CmdHandler=Handler; }

/** WinProcessMsgs() *****************************************/
/** Call this function periodically to process Windows      **/
/** messages. Returns number of messages processed.         **/
/*************************************************************/
int WinProcessMsgs(void)
{
  MSG Msg;
  int J;

  /* No messages yet */
  J=0;

  /* When saving CPU time, spin here until focus is in */
  do
  {
    /* Process all messages */
    for(;PeekMessage(&Msg,hWnd,0,0,PM_REMOVE);++J)
      DispatchMessage(&Msg);
    /* When saving CPU, wait */
    if(SaveCPU&&FocusOut)
    { GetMessage(&Msg,hWnd,0,0);DispatchMessage(&Msg); }
  }
  while(SaveCPU&&FocusOut);

  /* Done */
  return(J);
}

/** WinSetWindow() *******************************************/
/** Set window size and position. Returns 0 if failed.      **/
/*************************************************************/
int WinSetWindow(int X,int Y,int Width,int Height)
{
  RECT R;

  /* If requesting full screen... */
  if((Width<=0)||(Height<=0))
  {
    /* If it is already on, nothing to do */
    if(DDSurface&&DDFullScreen&&(DDWindow==hWnd)) return(1);
    /* Try initializing DirectDraw */
    if(!DDInit(hWnd,Width,Height)) return(0);
    return(1);
  }
  else
  {
    /* If were running full screen, shut it down */
    if(DDSurface&&DDFullScreen&&(DDWindow==hWnd)) DDTrash();
    /* Adjust window rectangle */
    R.top    = 0;
    R.left   = 0;
    R.right  = Width-1;
    R.bottom = Height-1;
    if(!AdjustWindowRect(&R,WS_OVERLAPPEDWINDOW,TRUE)) return(0);
    /* Set window position */
    Width    = R.right-R.left+1;
    Height   = R.bottom-R.top+1;
    return(SetWindowPos(hWnd,0,X,Y,Width,Height,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE));
  }
}

/** WinAskFilename() *****************************************/
/** Shows an open-file dialog and makes user select a file. **/
/** Returns 0 on failure. ATTENTION: This function returns  **/
/** pointer to the internal buffer!                         **/
/*************************************************************/
const char *WinAskFilename(const char *Title,const char *Types,unsigned int Options)
{
  OPENFILENAME OFN;
  char S[256],*P;
  int J;

  memset(&OFN,0,sizeof(OFN));
  OFN.lStructSize  = sizeof(OFN);
  OFN.hwndOwner    = hWnd;
  OFN.lpstrFilter  = Types? Types:"All Files (*.*)\0*.*\0";
  OFN.nFilterIndex = 1;
  OFN.lpstrFile    = Options&ASK_SAVEFILE? SaveFilename:OpenFilename;
  OFN.nMaxFile     = sizeof(OpenFilename)-1;
  OFN.lpstrTitle   = Title? Title:"Please Choose File";
  OFN.Flags        = OFN_PATHMUSTEXIST;
  if(!(Options&ASK_SAVEFILE? GetSaveFileName(&OFN):GetOpenFileName(&OFN)))
    return(0);

  if((Options&ASK_ASSOCIATE)&&!(Options&ASK_SAVEFILE))
  {
    /* Extract filename extension */
    P = strrchr(OpenFilename,'\\');
    P = strchr(P? P:OpenFilename,'.');
    /* If extension has been extracted but not yet associated... */
    if(P)
      if(!WinGetInteger(0,P,0))
      {
        /* Ask user to associate chosen extension to the program */
        sprintf(S,"Do you wish to always open %s files with %s?",P,AppClass);
        switch(MessageBox(hWnd,S,"Add Extension",MB_ICONQUESTION|MB_YESNOCANCEL))
        {
          case IDYES:
            if(WinAssociate(P,0,0)) WinSetInteger(0,P,1);
            break;
          case IDNO:
            WinSetInteger(0,P,2);
            break;
        }
      }
  }

  /* Return filename */
  return(Options&ASK_SAVEFILE? SaveFilename:OpenFilename);
}

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void)
{
  Image *Img;

  /* Must have active video image and a window */
  if(!VideoImg||!VideoImg->Data||!hWnd) return(0);

  Img=VideoImg;

  /* If image post-processing requested or 640x480 fullscreen... */
  if((Effects&(EFF_SOFTEN|EFF_TV))||(DDFullScreen==2))
  {
    /* Recreate image processing buffer if needed */
    if(!BigScreen.Data)
    {
      int W,H;
      /* Compute desired output dimensions */
      W = !DDFullScreen? AppWidth:DDFullScreen==2? 640:320;
      W = (AppWidth>W)||(Effects&EFF_SOFTEN)? W
        : (W/AppWidth)*AppWidth;
      H = !DDFullScreen? AppHeight:DDFullScreen==2? 480:240;
      H = (AppHeight>H)||(Effects&EFF_SOFTEN)? H
        : (H/AppHeight)*AppHeight;
      /* Adjust for correct width/height ratio */
      if((W*AppHeight)/AppWidth>H) W=(H*AppWidth)/AppHeight;
      else if((H*AppWidth)/AppHeight>W) H=(W*AppHeight)/AppWidth;
      /* Create new image processing buffer */
      NewImage(&BigScreen,W,H);
    }

    /* If image processing buffer present... */
    if(BigScreen.Data)
    {
      if(Effects&EFF_SOFTEN)
        SoftenImage(&BigScreen,VideoImg,VideoX,VideoY,VideoW,VideoH);
      else
        ScaleImage(&BigScreen,VideoImg,VideoX,VideoY,VideoW,VideoH);
      if(Effects&EFF_TV)
        TelevizeImage(&BigScreen,0,0,BigScreen.W,BigScreen.H);
    }
  }
  else
  {
    /* If only TV scanlines requested, apply them directly */
    if(Effects&EFF_TV) TelevizeImage(Img,VideoX,VideoY,VideoW,VideoH);
  }

  /* If timer synchronization requested, wait for timer */
  if(Effects&EFF_SYNC) WaitSyncTimer();
  /* Send message to repaint the window */ 
  SendMessage(hWnd,WM_PAINT,0,0);
  /* Let window process the message immediately */
  WinProcessMsgs();

  /* Done */
  return(1);
}

/** PutImageWH() *********************************************/
/** Show image in a window, resize it to DWxDH.             **/
/*************************************************************/
static int PutImageWH(Image *Img,int X,int Y,int W,int H,int DW,int DH)
{
  POINT P;
  RECT R;
  HDC hDDC;
  int J,SW,SH;

  /* Get window client rectangle */
  if(!DDSurface||!DDFullScreen||(DDWindow!=hWnd))
  {
    /* GetClientRect() returns one extra pixel in each direction */
    if(!GetClientRect(hWnd,&R)) return(0);
    if(DW<=0) DW=(int)(R.right-R.left);
    if(DH<=0) DH=(int)(R.bottom-R.top);
  }

  /* If DirectDraw surface exists... */
  if(DDSurface&&(DDWindow==hWnd))
  {
    if(IDirectDrawSurface7_IsLost(DDSurface)==DDERR_SURFACELOST)
      if(IDirectDrawSurface7_Restore(DDSurface)!=DD_OK) return(0);

    if(IDirectDrawSurface7_GetDC(DDSurface,&hDDC)!=DD_OK) return(0);

    if(DDFullScreen)
    {
      SW = DDFullScreen==2? 640:320;
      SH = DDFullScreen==2? 480:240;
      DW = W; //SW<=W? W:(int)(SW/W)*W;
      DH = H; //SH<=H? H:(int)(SH/H)*H;

      BitBlt(hDDC,0,0,(SW-DW)/2,SH,NULL,0,0,BLACKNESS);
      BitBlt(hDDC,(SW+DW)/2,0,(SW-DW)/2,SH,NULL,0,0,BLACKNESS);
      BitBlt(hDDC,(SW-DW)/2,0,DW,(SH-DH)/2,NULL,0,0,BLACKNESS);
      BitBlt(hDDC,(SW-DW)/2,(SH+DH)/2,SW,(SH-DH)/2,NULL,0,0,BLACKNESS);

      J  = (DW==W)&&(DH==H)?
           BitBlt(hDDC,(SW-W)/2,(SH-H)/2,W,H,Img->hDC,X,Y,SRCCOPY)
         : StretchBlt(hDDC,(SW-DW)/2,(SH-DH)/2,DW,DH,Img->hDC,X,Y,W,H,SRCCOPY);
    }
    else
    {
      P.x=R.left;
      P.y=R.top;
      ClientToScreen(hWnd,&P);
      J=StretchBlt(
        hDDC,P.x+(R.right+R.left-DW)/2,P.y+(R.bottom+R.top-DH)/2,DW,DH,
        Img->hDC,X,Y,W,H,
        SRCCOPY
      );
    }

    /* Done */
    IDirectDrawSurface7_ReleaseDC(DDSurface,hDDC);
    return(J);
  }

  /* No DirectDraw, using GDI instead... */

  /* Get device context */
  if(!(hDDC=GetDC(hWnd))) return(0);

  /* Blit */
  J=StretchBlt(
    hDDC,(R.right+R.left-DW)/2,(R.bottom+R.top-DH)/2,DW,DH,
    Img->hDC,X,Y,W,H,
    SRCCOPY
  );

  /* Done */
  ReleaseDC(hWnd,hDDC);
  return(J);
}

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent). The     **/
/** Delay value is given in millseconds and determines how  **/
/** much buffering will be done.                            **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Delay)
{
  WAVEFORMATEX Format;
  HANDLE hThread;
  int I;

  /* Clean up previous sound setup */
  TrashAudio();

  /* Initialize stuff */
  ThreadID  = 0;
  hWave     = 0;
  SndRate   = 0;
  SndSize   = 0;
  SndBuffer = 0;
  CurHdr    = 0;

  /* When Rate=0, be silent */
  if(!Rate) return(0);

  /* Allocate enough buffer space */
  SndSize   = Rate*Delay/1000/SND_HEADERS;
  SndSize   = (SndSize<SND_MINBLOCK? SND_MINBLOCK:SndSize)*SND_HEADERS;
  SndBuffer = malloc(sizeof(sample)*SndSize);
  if(!SndBuffer) return(0);

  /* Initialize pointers into the buffer */
  memset(SndBuffer,0,sizeof(sample)*SndSize);
  SndWPtr = SndBuffer;
  SndRPtr = SndBuffer+SndSize/2;

  /* Create an audio thread */
  hThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadFunc,0,0,&ThreadID);
  if(!hThread) { free(SndBuffer);SndBuffer=0;return(0); }
  
  /* Sound update thread is time critical */
  SetThreadPriority(hThread,THREAD_PRIORITY_TIME_CRITICAL);
  /* No longer need the handle, thread will exit by itself */ 
  CloseHandle(hThread);
  /* Let the thread go into message loop */
  Sleep(100);

  /* Set required format: PCM-16bit-Mono */
  Format.wFormatTag      = WAVE_FORMAT_PCM;
  Format.nChannels       = 1;
  Format.nSamplesPerSec  = Rate;
  Format.wBitsPerSample  = sizeof(sample)*8;
  Format.nBlockAlign     = Format.nChannels*Format.wBitsPerSample/8;
  Format.nAvgBytesPerSec = Format.nSamplesPerSec*Format.nBlockAlign;
  Format.cbSize          = 0;

  /* Open WaveOut device */
  if(waveOutOpen(&hWave,WAVE_MAPPER,&Format,ThreadID,0,CALLBACK_THREAD))
  {
    PostThreadMessage(ThreadID,MM_WOM_CLOSE,(WPARAM)hWave,0);
    free(SndBuffer);
    SndBuffer=0;
    return(0);
  }

  /* Clear all buffer headers */
  for(I=0;I<SND_HEADERS;I++) SndHeader[I].dwFlags=0;

  /* Enqueue buffers ready to play */
  PlayBuffers();

  /* Success */
  SndRate=Rate;
  return(Rate);
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
  if(SndRate)
  {
    if(ThreadID)  { PostThreadMessage(ThreadID,MM_WOM_CLOSE,(WPARAM)hWave,0);ThreadID=0; }
    if(hWave)     { waveOutReset(hWave);waveOutClose(hWave);hWave=0; }
    if(SndBuffer) { free(SndBuffer);SndBuffer=0; }
    SndRate=0;
  } 
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{ return(SndRPtr>=SndWPtr? SndRPtr-SndWPtr:SndRPtr-SndWPtr+SndSize-1); }

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
  unsigned int J;

  /* Can't write more than the free space */
  J=GetFreeAudio();
  if(J<Length) Length=J;

  /* Copy data */
  for(J=0;J<Length;J++)
  {
    *SndWPtr++=Data[J];
    if(SndWPtr-SndBuffer>=SndSize) SndWPtr=SndBuffer;
  }

  /* Done */
  return(Length);
}

/** WinGetJoysticks() ****************************************/
/** Get the state of *real* joypad buttons (1="pressed"),   **/
/** but *not* the keyboard keys. Refer to BTN_* #defines    **/
/** for the button mappings.                                **/
/*************************************************************/
unsigned int WinGetJoysticks(void)
{
  unsigned int J;
  JOYINFOEX JS;
  int N,I;

  /* Check joystick(s) */
  JS.dwSize  = sizeof(JS);
  JS.dwFlags = JOY_RETURNBUTTONS|JOY_RETURNX|JOY_RETURNY;
  for(I=JoyCount-1,J=0;I>=0;--I)
    if(joyGetPosEx(JOYSTICKID1+I,&JS)==JOYERR_NOERROR)
    {
      J<<=16;
      N=(JoyCaps[I].wXmax-JoyCaps[I].wXmin)/3;
      if(JS.dwXpos<JoyCaps[I].wXmin+N)      J|=BTN_LEFT;
      else if(JS.dwXpos>JoyCaps[I].wXmax-N) J|=BTN_RIGHT;
      N=(JoyCaps[I].wYmax-JoyCaps[I].wYmin)/3;
      if(JS.dwYpos<JoyCaps[I].wYmin+N)      J|=BTN_UP;
      else if(JS.dwYpos>JoyCaps[I].wYmax-N) J|=BTN_DOWN;
      if(JS.dwButtons&JoyButtons[I][0])     J|=BTN_FIREA;
      if(JS.dwButtons&JoyButtons[I][1])     J|=BTN_FIREB;
      if(JS.dwButtons&JoyButtons[I][2])     J|=BTN_FIREL;
      if(JS.dwButtons&JoyButtons[I][3])     J|=BTN_FIRER;
      if(JS.dwButtons&JoyButtons[I][4])     J|=BTN_START;
      if(JS.dwButtons&JoyButtons[I][5])     J|=BTN_SELECT;
      if(JS.dwButtons&JoyButtons[I][6])     J|=BTN_EXIT;
    }

  /* Return joystick state + keyboard modifiers */
  return(J|KeyModes);
}

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings. Notice that **/
/** on Windows this function calls WinProcessMsgs() thus    **/
/** automatically handling all Windows messages.            **/
/*************************************************************/
unsigned int GetJoystick(void)
{
  unsigned int J,I;

  /* Process Windows messages */
  WinProcessMsgs();

  /* Compute combined joystick state */
  J=JoyState|WinGetJoysticks()|KeyModes;

  /* Swap players #1/#2 when ALT button pressed */
  if(J&BTN_ALT) J=(J&BTN_MODES)|((J>>16)&BTN_ALL)|((J&BTN_ALL)<<16);

  /* Return combined joystick state */
  return(J);
}

/** GetMouse() ***********************************************/
/** Get mouse position and button states in the following   **/
/** format: RMB.LMB.Y[29-16].X[15-0]                        **/
/*************************************************************/
unsigned int GetMouse(void)
{
  /* Return mouse state */
  return(MousePos);
}

/** GetKey() *************************************************/
/** Get currently pressed key or 0 if none pressed. Returns **/
/** CON_* definitions for arrows and special keys.          **/
/*************************************************************/
unsigned int GetKey(void)
{
  unsigned int J;

  /* Process Windows messages */
  WinProcessMsgs();
  /* Get last key code */
  J=LastKey;
  /* Reset last key code */
  LastKey=0;
  /* Done */
  return(J|KeyModes);
}

/** WaitKey() ************************************************/
/** Wait for a key to be pressed. Returns CON_* definitions **/
/** for arrows and special keys.                            **/
/*************************************************************/
unsigned int WaitKey(void)
{
  unsigned int J;

  /* Wait for a key to be pressed */
  do
  {
    WaitMessage();
    J=GetKey();
  }
  while(!(J&0xFFFF)&&VideoImg);
  /* Done */
  return(J);
}

/** SyncTimerReady() *****************************************/
/** Return 1 if sync timer ready, 0 otherwise.              **/
/*************************************************************/
int SyncTimerReady(void)
{
  return(!TimerID||TimerReady);
}

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready.                     **/
/*************************************************************/
void WaitSyncTimer(void)
{
  while(TimerID&&!TimerReady) { WinProcessMsgs();Sleep(1); }
  TimerReady=0;
}

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz)
{
  /* Kill existing timer */
  if(TimerID) timeKillEvent(TimerID);

  /* Set new timer */
  if(Hz<1) TimerID=0;
  else
  {
    /* If timer was off, start timer period */
    if(!TimerID) timeBeginPeriod(5);

    /* Set timer */
    TimerID=timeSetEvent(1000/Hz,333/Hz,TimerProc,0,TIME_PERIODIC);
  }

  /* If timer is turned off, stop timer period */
  if(!TimerID) timeEndPeriod(5);

  /* Return allocated timer ID */
  return(TimerID);
}

/** ChangeDir() **********************************************/
/** This function is a wrapper for chdir(). Unlike chdir(), **/
/** it changes *both* the drive and the directory in MSDOS, **/
/** exactly like the "normal" chdir() should. Returns 0 on  **/
/** success, -1 on failure, just like chdir().              **/
/*************************************************************/
int ChangeDir(const char *Name)
{
  // @@@ WRITE CODE HERE!
  return(0);
}

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height)
{
  struct
  {
    BITMAPINFOHEADER Header;
    DWORD Colors[256];
  } BMapInfo;
  HDC hDC;
  HBITMAP hBMap;
  pixel *Data;
  int J;

  /* No image yet */
  Img->Data    = 0;
  Img->Cropped = 0;
  Img->hDC     = 0;
  Img->hBMap   = 0;
  Img->W       = 0;
  Img->H       = 0;
  Img->L       = 0;
  Img->D       = 0;

  /* Create DC compatible with the screen */
  hDC=CreateCompatibleDC(0);
  if(!hDC) return(0);

  /* Fill out BITMAPINFO structure */
  memset(&BMapInfo,0,sizeof(BMapInfo));
  BMapInfo.Header.biSize         = sizeof(BITMAPINFOHEADER);
  BMapInfo.Header.biWidth        = Width;
  BMapInfo.Header.biHeight       = -Height;
  BMapInfo.Header.biBitCount     = sizeof(pixel)*8;
  BMapInfo.Header.biCompression  = BI_RGB;
  BMapInfo.Header.biClrUsed      = 0;
  BMapInfo.Header.biClrImportant = 0;
  BMapInfo.Header.biPlanes       = 1;
  BMapInfo.Header.biSizeImage    = 0;

#if !defined(BPP16) && !defined(BPP32)
  /* Create a static 8bit palette */
  BMapInfo.Header.biClrUsed      = 256;
  BMapInfo.Header.biClrImportant = 256;
  for(J=0;J<256;++J)
    BMapInfo.Colors[J]=RGB(255*(J&3)/3,255*((J>>2)&7)/7,255*(J>>5)/7);
#endif

  /* Create DIB bitmap */
  hBMap=CreateDIBSection(hDC,(BITMAPINFO *)&BMapInfo,DIB_RGB_COLORS,&Data,0,0);
  if(!hBMap)
  {
    DeleteDC(hDC);
    return(0);
  }

  /* Select bitmap into DC */
  if(!SelectObject(hDC,hBMap))
  {
    DeleteObject(hBMap);
    DeleteDC(hDC);
    return(0);
  }

  /* Compute pitch */
  J = ((Width*sizeof(pixel)+3)&~3)/sizeof(pixel);

  /* Clear bitmap */
  memset(Data,0,J*Height*sizeof(pixel));

  /* Done */
  Img->hDC    = hDC;
  Img->hBMap  = hBMap;
  Img->W      = Width;
  Img->H      = Height;
  Img->L      = J;
  Img->D      = sizeof(pixel);
  Img->Data   = Data;
  return(Data); 
}

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img)
{
  if(!Img->Cropped)
  {
    if(Img->hDC)   DeleteDC(Img->hDC); 
    if(Img->hBMap) DeleteObject(Img->hBMap);
  }
  Img->hDC     = 0;
  Img->hBMap   = 0;
  Img->Data    = 0;
  Img->Cropped = 0;
  Img->W       = 0;
  Img->H       = 0;
  Img->L       = 0;
  Img->D       = 0;
}

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  GenericCropImage(Dst,Src,X,Y,W,H);
  Dst->hDC   = 0;
  Dst->hBMap = 0;
  return(Dst);
}

/** WinMain() ************************************************/
/** This is a main function from which Windows starts       **/
/** executing the program.                                  **/
/*************************************************************/
int PASCAL WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR CmdLine,int CmdShow)
{
  WNDCLASS WndClass;
  BITMAPINFO BMapInfo;
  WSADATA WSAInfo;
  RECT R;
  char *P,**argv;
  int J,Quote;
  
  VideoImg     = 0;
  JoyState     = 0x00000000;
  MousePos     = 0x00000000;
  LastKey      = 0;
  KeyModes     = 0;
  SaveCPU      = 0;
  FullScreen   = 0;
  Effects      = 0;
  FocusOut     = 0;
  hInst        = hInstance;
  hWnd         = 0;
  hWave        = 0;
  CurHdr       = 0;
  SndWPtr      = SndBuffer+1;
  SndRPtr      = SndBuffer;
  ThreadID     = 0;
  TimerID      = 0;
  CmdHandler   = 0;
  ISDelay      = 100;
  ISRate       = 22050;
  BackupSwitch = 0x0000;

  BigScreen.Data  = 0;
  OpenFilename[0] = '\0';
  SaveFilename[0] = '\0';
  strcpy(MIDIName,"LOG.MID");

  WndClass.style         = CS_BYTEALIGNCLIENT|CS_HREDRAW|CS_VREDRAW;
  WndClass.lpfnWndProc   = WndProc;
  WndClass.cbClsExtra    = 0;
  WndClass.cbWndExtra    = 0;
  WndClass.hInstance     = hInstance;
  WndClass.hIcon         = LoadIcon(hInstance,"Icon");
  WndClass.hCursor       = LoadCursor(0,IDC_ARROW);
  WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  WndClass.lpszMenuName  = "Menu";
  WndClass.lpszClassName = AppClass;
  if(!RegisterClass(&WndClass)) return(0);

  /* Get window dimensions */
  AppWidth   = WinGetInteger(0,"Width",AppWidth);
  AppHeight  = WinGetInteger(0,"Height",AppHeight);
  SaveCPU    = WinGetInteger(0,"SaveCPU",SaveCPU);
  Effects    = WinGetInteger(0,"Effects",Effects);
//  FullScreen = WinGetInteger(0,"FullScreen",FullScreen);
  WinGetString(0,"Filename",OpenFilename,sizeof(OpenFilename));
 
  /* Get desktop size */
  GetWindowRect(GetDesktopWindow(),(LPRECT)&R);
  AppX=R.right-R.left+1;
  AppY=R.bottom-R.top+1;

  /* Adjust window rectangle */
  R.top    = 0;
  R.left   = 0;
  R.right  = AppWidth-1;
  R.bottom = AppHeight-1;
  AdjustWindowRect(&R,WS_OVERLAPPEDWINDOW,TRUE);

  /* Place window at the middle of the screen */
  AppX=(AppX-R.right+R.left+1)/2;
  AppY=(AppY-R.bottom+R.top+1)/2;

  /* Get window position */
  AppX = WinGetInteger(0,"X",AppX);
  AppY = WinGetInteger(0,"Y",AppY);

  /* Create window */
  hWnd=CreateWindowEx(
    WS_EX_COMPOSITED,
    AppClass,AppTitle,
    WS_VISIBLE|WS_OVERLAPPEDWINDOW,
    AppX,AppY,R.right-R.left+1,R.bottom-R.top+1,
    0,0,hInstance,0
  );
  /* If composited window fails (Win2000), try normal window */
  if(!hWnd)
    hWnd=CreateWindowEx(
      0,
      AppClass,AppTitle,
      WS_VISIBLE|WS_OVERLAPPEDWINDOW,
      AppX,AppY,R.right-R.left+1,R.bottom-R.top+1,
      0,0,hInstance,0
    );
  if(!hWnd) return(0);

  /* Get joystick capabilities */
  JoyCount=joyGetNumDevs();
  JoyCount=JoyCount<=2? JoyCount:2;
  for(J=0;J<JoyCount;++J)
    if(joyGetDevCaps(JOYSTICKID1+J,&JoyCaps[J],sizeof(JoyCaps[J]))!=JOYERR_NOERROR)
    { JoyCount=J;break; }

  /* Get joystick button mappings */
  JoyButtons[0][0] = WinGetInteger(0,"BtnFIREA1",JoyButtons[0][0]);
  JoyButtons[0][1] = WinGetInteger(0,"BtnFIREB1",JoyButtons[0][1]);
  JoyButtons[0][2] = WinGetInteger(0,"BtnFIREL1",JoyButtons[0][2]);
  JoyButtons[0][3] = WinGetInteger(0,"BtnFIRER1",JoyButtons[0][3]);
  JoyButtons[0][4] = WinGetInteger(0,"BtnSTART1",JoyButtons[0][4]);
  JoyButtons[0][5] = WinGetInteger(0,"BtnSELECT1",JoyButtons[0][5]);
  JoyButtons[0][6] = WinGetInteger(0,"BtnEXIT1",JoyButtons[0][6]);
  JoyButtons[1][0] = WinGetInteger(0,"BtnFIREA2",JoyButtons[1][0]);
  JoyButtons[1][1] = WinGetInteger(0,"BtnFIREB2",JoyButtons[1][1]);
  JoyButtons[1][2] = WinGetInteger(0,"BtnFIREL2",JoyButtons[1][2]);
  JoyButtons[1][3] = WinGetInteger(0,"BtnFIRER2",JoyButtons[1][3]);
  JoyButtons[1][4] = WinGetInteger(0,"BtnSTART2",JoyButtons[1][4]);
  JoyButtons[1][5] = WinGetInteger(0,"BtnSELECT2",JoyButtons[1][5]);
  JoyButtons[1][6] = WinGetInteger(0,"BtnEXIT2",JoyButtons[1][6]);

  /* Initialize sound */
  SetSoundSettings(
    WinGetInteger(0,"SndRate",ISRate),
    WinGetInteger(0,"SndDelay",ISDelay)
  );

  /* Initialize networking */
  WSAStartup(MAKEWORD(2,2),&WSAInfo);

  /* Check all needed menu items */
  CheckMenuItem(GetMenu(hWnd),WMENU_SAVECPU,SaveCPU? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(hWnd),WMENU_NETPLAY,NETPlay(NET_QUERY)? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(hWnd),WMENU_SOFTEN,Effects&EFF_SOFTEN? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(hWnd),WMENU_TV,Effects&EFF_TV? MF_CHECKED:MF_UNCHECKED);
  CheckMenuItem(GetMenu(hWnd),WMENU_SYNC,Effects&EFF_SYNC? MF_CHECKED:MF_UNCHECKED);

  /* Set full screen mode if needed */
//  if(FullScreen&&!WinSetWindow(AppX,AppY,AppMode,AppMode)) FullScreen=0;

  /* Allocate space for argv[] */
  for(P=CmdLine,J=2;*P;)
  {
    while(*P&&(*P<=' ')) P++;
    if(*P) J++;
    while(*P&&(*P>' ')) P++;
  }  
  argv=malloc(J*sizeof(char *));

  /* First argument is the full filename */
  argv[0]=GetModuleFileName(0,Fullpath,sizeof(Fullpath))? Fullpath:(char *)AppClass;

  /* Parse command line */
  for(P=CmdLine,J=1,Quote=0;*P;)
  {
    while(*P&&(*P<=' ')) P++;
    if(*P=='\"') { Quote=1;P++; }
    if(*P) argv[J++]=P;
    if(Quote) while(*P&&(*P!='\"')) P++;
    else      while(*P&&(*P>' ')) P++;
    if(*P) *P++='\0';
  }
  argv[J]=0;

  /* Call main program */
  Application(J,argv);

  /* Done */
  if(argv) free(argv);
  FreeImage(&BigScreen);
  NETClose();
  WSACleanup();
  return(1);
}

/** WndProc() ************************************************/
/** Main window handler.                                    **/
/*************************************************************/
static LRESULT CALLBACK WndProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
  static int SndRates[]  = { 0,1,11025,22050,44100 };
  static int SndDelays[] = { 10,50,100,250,500 };
  char S[300],T[256],*P;
  PAINTSTRUCT PS;
  RECT R;
  int J,X,Y;

  switch(Msg)
  {
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;

    case WM_SIZE:
      /* Remove current video effects buffer */
      FreeImage(&BigScreen);
      /* Unless running full screen... */
      if(!FullScreen)
      {
        /* Save window size to AppWidth/AppHeight */
        AppWidth  = LOWORD(lParam);
        AppHeight = HIWORD(lParam);
      }
      break;

    case WM_MOVE:
      /* Save window position to AppX/AppY */
      if(!FullScreen)
      {
        GetWindowRect(hWnd,(LPRECT)&R);
        AppX=R.left;
        AppY=R.top;
      }
      break;

    case WM_PAINT:
      BeginPaint(hWnd,&PS);
      /* If video buffer present */
      if(VideoImg&&VideoImg->Data)
      {
        /* If video effects of 640x480 fullscreen, us processing buffer */
        if(BigScreen.Data&&((Effects&(EFF_SOFTEN|EFF_TV))||(DDFullScreen==2)))
          PutImageWH(&BigScreen,0,0,BigScreen.W,BigScreen.H,0,0);
        else
          PutImageWH(VideoImg,VideoX,VideoY,VideoW,VideoH,0,0);
      }
      EndPaint(hWnd,&PS);
      break;

    case WM_DESTROY:
      // Save settings 
      WinSetInteger(0,"X",AppX);
      WinSetInteger(0,"Y",AppY);
      WinSetInteger(0,"Width",AppWidth);
      WinSetInteger(0,"Height",AppHeight);
      WinSetInteger(0,"FullScreen",FullScreen);
      WinSetInteger(0,"Effects",Effects);
      WinSetInteger(0,"SaveCPU",SaveCPU);
      WinSetInteger(0,"SndRate",ISRate);
      WinSetInteger(0,"SndDelay",ISDelay);
      WinSetString(0,"Filename",OpenFilename);
      WinSetInteger(0,"BtnFIREA1",JoyButtons[0][0]);
      WinSetInteger(0,"BtnFIREB1",JoyButtons[0][1]);
      WinSetInteger(0,"BtnFIREL1",JoyButtons[0][2]);
      WinSetInteger(0,"BtnFIRER1",JoyButtons[0][3]);
      WinSetInteger(0,"BtnSTART1",JoyButtons[0][4]);
      WinSetInteger(0,"BtnSELECT1",JoyButtons[0][5]);
      WinSetInteger(0,"BtnEXIT1",JoyButtons[0][6]);
      WinSetInteger(0,"BtnFIREA2",JoyButtons[1][0]);
      WinSetInteger(0,"BtnFIREB2",JoyButtons[1][1]);
      WinSetInteger(0,"BtnFIREL2",JoyButtons[1][2]);
      WinSetInteger(0,"BtnFIRER2",JoyButtons[1][3]);
      WinSetInteger(0,"BtnSTART2",JoyButtons[1][4]);
      WinSetInteger(0,"BtnSELECT2",JoyButtons[1][5]);
      WinSetInteger(0,"BtnEXIT2",JoyButtons[1][6]);
      // Shut down sound
      TrashSound();
      // Reset timer
      SetSyncTimer(0);
      // Force focus back in
      FocusOut=0;
      // Close console
      VideoImg=0;
      // Signal program exit to the application
      if(!CmdHandler) ExitProcess(0); else (*CmdHandler)(WMENU_QUIT);
      break;

    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
      // Shut down sound when resizing window or showing menu
      BackupSwitch=MasterSwitch;
      SetChannels(MasterVolume,0x0000);
      break;

    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
      // Restore sound after resizing or the menu
      SetChannels(MasterVolume,BackupSwitch);
      break;

    case WM_ACTIVATE:
      if(LOWORD(wParam)!=WA_INACTIVE)
      {
        // Restore sound when focus enters window
        SetChannels(MasterVolume,BackupSwitch);
        FocusOut=0;
      }
      else
      {
        // Shut down sound when focus leaves window
        BackupSwitch=MasterSwitch;
        SetChannels(MasterVolume,0x0000);
        FocusOut=1;
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
      X = LOWORD(lParam); 
      Y = HIWORD(lParam); 
      X = X<0? 0:X>=AppWidth? AppWidth-1:X;
      Y = Y<0? 0:Y>=AppHeight? AppHeight-1:Y;
      MousePos = X
               | (Y<<16)
               | (wParam&MK_LBUTTON? 0x40000000:0)
               | (wParam&MK_RBUTTON? 0x80000000:0);
      break;
        
    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case WMENU_SNDOFF:
        case WMENU_SNDMIDI:
        case WMENU_SND11kHz:
        case WMENU_SND22kHz:
        case WMENU_SND44kHz:
          SetSoundSettings(SndRates[LOWORD(wParam)-WMENU_SNDOFF],ISDelay);
          break;
        case WMENU_SND10ms:
        case WMENU_SND50ms:
        case WMENU_SND100ms:
        case WMENU_SND250ms:
        case WMENU_SND500ms:
          SetSoundSettings(ISRate,SndDelays[LOWORD(wParam)-WMENU_SND10ms]);
          break;
        case WMENU_SIZEx1:
        case WMENU_SIZEx2:
        case WMENU_SIZEx3:
        case WMENU_SIZEx4:
          FullScreen = 0;
          AppWidth   = VideoW*(LOWORD(wParam)-WMENU_SIZEx1+1);
          AppHeight  = VideoH*(LOWORD(wParam)-WMENU_SIZEx1+1);
          WinSetWindow(AppX,AppY,AppWidth,AppHeight);
          break;
        case WMENU_TV:
          Effects^=EFF_TV;
          CheckMenuItem(GetMenu(hWnd),WMENU_TV,Effects&EFF_TV? MF_CHECKED:MF_UNCHECKED);
          break;
        case WMENU_SOFTEN:
          Effects^=EFF_SOFTEN;
          CheckMenuItem(GetMenu(hWnd),WMENU_SOFTEN,Effects&EFF_SOFTEN? MF_CHECKED:MF_UNCHECKED);
          break;
        case WMENU_SYNC:
          Effects^=EFF_SYNC;
          CheckMenuItem(GetMenu(hWnd),WMENU_SYNC,Effects&EFF_SYNC? MF_CHECKED:MF_UNCHECKED);
          break;
        case WMENU_QUIT:
          DestroyWindow(hWnd);
          break;
        case WMENU_FULL:
          /* Filp fullscreen mode */
          FullScreen=!FullScreen;
          if(!WinSetWindow(AppX,AppY,FullScreen? AppMode:AppWidth,FullScreen? AppMode:AppHeight))
            FullScreen=!FullScreen;
          else
            FreeImage(&BigScreen);
          break;
        case WMENU_SAVECPU:
          SaveCPU=!SaveCPU;
          CheckMenuItem(GetMenu(hWnd),WMENU_SAVECPU,SaveCPU? MF_CHECKED:MF_UNCHECKED);
          break;
        case WMENU_SNDLOG:
          MIDILogging(MIDI_TOGGLE);
          CheckMenuItem(GetMenu(hWnd),WMENU_SNDLOG,MIDILogging(MIDI_QUERY)? MF_CHECKED:MF_UNCHECKED);
          break;
        case WMENU_SaveMID:
          // Get MIDI file name
          P=(char *)WinAskFilename(
            "Save MIDI Sountrack",
            "MIDI Files (*.mid,*.rmi)\0*.mid;*.rmi\0All Files (*.*)\0*.*\0",
            ASK_SAVEFILE
          );
          // If MIDI file selected...
          if(P)
          {
            // Copy filename
            strcpy(MIDIName,P);
            // Initialize MIDI logging and turn it on
            InitMIDI(MIDIName);
            MIDILogging(MIDI_ON); 
            // Check menu item
            CheckMenuItem(GetMenu(hWnd),WMENU_SNDLOG,MIDILogging(MIDI_QUERY)? MF_CHECKED:MF_UNCHECKED);
          }
          break;
        case WMENU_HELP:
          GetModuleFileName(0,T,sizeof(T));
          P  = strrchr(T,'\\');
          P  = P? P+1:T;
          *P = '\0';
          sprintf(S,
            "rundll32 url.dll,FileProtocolHandler \"%s\\%s.html\"",
            T,AppClass
          );
          WinExec(S,SW_SHOW);
          break;
        case WMENU_WEB:
          sprintf(S,
            "rundll32 url.dll,FileProtocolHandler \""HOMEPAGE"%s/\"",
            AppClass
          );
          WinExec(S,SW_SHOW);
          break;
        case WMENU_AUTHOR:
          WinExec("rundll32 url.dll,FileProtocolHandler \""HOMEPAGE"\"",SW_SHOW);
          break;
        case WMENU_NETPLAY:
          /* Toggle NetPlay, do not relay WMENU_NETPLAY to */
          /* application if connection has failed          */
          J=NETPlay(NET_QUERY);
          if(NETPlay(NET_TOGGLE)==J) return(0);
          /* Check menu option */
          CheckMenuItem(GetMenu(hWnd),WMENU_NETPLAY,NETPlay(NET_QUERY)? MF_CHECKED:MF_UNCHECKED);
          break;
      }
      /* Forward command to the application */
      if(CmdHandler) (*CmdHandler)(LOWORD(wParam));
      break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
      /* Key just went up */
      /* Check ALT key status */
      if(HIWORD(lParam)&KF_ALTDOWN) KeyModes|=CON_ALT; else KeyModes&=~CON_ALT;
      /* Call internal LibWin handler */
      HandleKeys((int)wParam,0);
      /* Call user handler */
      if(KeyHandler) (*KeyHandler)(wParam|CON_RELEASE|KeyModes);
      break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      /* Key just went down */
      /* Check for special keys */
      if(HIWORD(lParam)&KF_ALTDOWN)
        switch(wParam)
        {
          case VK_RETURN:
            if(!(HIWORD(lParam)&KF_REPEAT))
            {
              FullScreen=!FullScreen;
              if(!WinSetWindow(AppX,AppY,FullScreen? AppMode:AppWidth,FullScreen? AppMode:AppHeight))
                FullScreen=!FullScreen;
              else
                FreeImage(&BigScreen);
            }
            return(0);
          case VK_PRIOR:
            SetChannels(MasterVolume<255-8? MasterVolume+8:255,MasterSwitch);
            return(0);
          case VK_NEXT:
            SetChannels(MasterVolume>8? MasterVolume-8:0,MasterSwitch);
            return(0);
          case VK_F4:
            DestroyWindow(hWnd);
            return(0);
        }
      /* Check ALT key status */
      if(HIWORD(lParam)&KF_ALTDOWN) KeyModes|=CON_ALT; else KeyModes&=~CON_ALT;
      /* Call internal LibWin handler */
      HandleKeys((int)wParam,1);
      /* Call user handler */
      if(KeyHandler) (*KeyHandler)(wParam|KeyModes);
      break;

    default:
      return(DefWindowProc(hWnd,Msg,wParam,lParam));
  }

  return(0);
}

/** HandleKeys() *********************************************/
/** Handle key presses/releases. Returns 1 if a key was     **/
/** recognized, 0 otherwise.                                **/
/*************************************************************/
static int HandleKeys(int Key,int Down)
{
  if(Down)
  {
    /* Produce key codes for GetKey() */
    if(((Key>='0')&&(Key<='9'))||((Key>='A')&&(Key<='Z'))) LastKey=Key;

    switch(Key)
    {
      case VK_SHIFT:   KeyModes|=CON_SHIFT;break;
      case VK_CONTROL: KeyModes|=CON_CONTROL;break;
      case VK_CAPITAL: KeyModes|=CON_CAPS;break;
      case VK_F1:      LastKey=CON_F1;break;
      case VK_F2:      LastKey=CON_F2;break;
      case VK_F3:      LastKey=CON_F3;break;
      case VK_F4:      LastKey=CON_F4;break;
      case VK_F5:      LastKey=CON_F5;break;
      case VK_F6:      LastKey=CON_F6;break;
      case VK_F7:      LastKey=CON_F7;break;
      case VK_F8:      LastKey=CON_F8;break;
      case VK_F9:      LastKey=CON_F9;break;
      case VK_F10:     LastKey=CON_F10;break;
      case VK_F11:     LastKey=CON_F11;break;
// This appears to happen by default in Windows!
//      case VK_F12:     LastKey=CON_F12;DestroyWindow(hWnd);break;
      case VK_LEFT:    JoyState|=BTN_LEFT;LastKey=CON_LEFT;break;
      case VK_RIGHT:   JoyState|=BTN_RIGHT;LastKey=CON_RIGHT;break;
      case VK_UP:      JoyState|=BTN_UP;LastKey=CON_UP;break;
      case VK_DOWN:    JoyState|=BTN_DOWN;LastKey=CON_DOWN;break;
      case VK_RETURN:  JoyState|=BTN_START;LastKey=CON_OK;break;
      case VK_ESCAPE:  JoyState|=BTN_EXIT;LastKey=CON_EXIT;break;
      case VK_TAB:     JoyState|=BTN_SELECT;LastKey=0x09;break;
      case VK_BACK:    LastKey=0x08;break;
      case '1':        LastKey=KeyModes&CON_SHIFT? '!':'1';break;
      case '2':        LastKey=KeyModes&CON_SHIFT? '@':'2';break;
      case '3':        LastKey=KeyModes&CON_SHIFT? '#':'3';break;
      case '4':        LastKey=KeyModes&CON_SHIFT? '$':'4';break;
      case '5':        LastKey=KeyModes&CON_SHIFT? '%':'5';break;
      case '6':        LastKey=KeyModes&CON_SHIFT? '^':'6';break;
      case '7':        LastKey=KeyModes&CON_SHIFT? '&':'7';break;
      case '8':        LastKey=KeyModes&CON_SHIFT? '*':'8';break;
      case '9':        LastKey=KeyModes&CON_SHIFT? '(':'9';break;
      case '0':        LastKey=KeyModes&CON_SHIFT? ')':'0';break;

      case VK_OEM_COMMA:  LastKey=KeyModes&CON_SHIFT? '<':',';break;
      case VK_OEM_PERIOD: LastKey=KeyModes&CON_SHIFT? '>':'.';break;
      case VK_OEM_MINUS:  LastKey=KeyModes&CON_SHIFT? '_':'-';break;
      case VK_OEM_PLUS:   LastKey=KeyModes&CON_SHIFT? '+':'=';break;
      case VK_OEM_1:      LastKey=KeyModes&CON_SHIFT? ':':';';break;
      case VK_OEM_2:      LastKey=KeyModes&CON_SHIFT? '?':'/';break;
      case VK_OEM_3:      LastKey=KeyModes&CON_SHIFT? '~':'`';break;
      case VK_OEM_4:      LastKey=KeyModes&CON_SHIFT? '{':'[';break;
      case VK_OEM_5:      LastKey=KeyModes&CON_SHIFT? '|':'\\';break;
      case VK_OEM_6:      LastKey=KeyModes&CON_SHIFT? '}':']';break;
      case VK_OEM_7:      LastKey=KeyModes&CON_SHIFT? '\"':'\'';break;

      case 'Q': case 'E': case 'T': case 'U': case 'O':
        JoyState|=BTN_FIREL;break;
      case 'W': case 'R': case 'Y': case 'I': case 'P':
        JoyState|=BTN_FIRER;break;
      case 'A': case 'S': case 'D': case 'F': case 'G':
      case 'H': case 'J': case 'K': case 'L':
        JoyState|=BTN_FIREA;break;
      case VK_SPACE:
        JoyState|=BTN_FIREA;LastKey=' ';break;
      case 'Z': case 'X': case 'C': case 'V': case 'B':
      case 'N': case 'M':
        JoyState|=BTN_FIREB;break;

      default:
        return(0);
    }
  }
  else
  {
    LastKey=0;
    switch(Key)
    {
      case VK_SHIFT:   KeyModes&=~CON_SHIFT;break;
      case VK_CONTROL: KeyModes&=~CON_CONTROL;break;
      case VK_CAPITAL: KeyModes&=~CON_CAPS;break;
      case VK_F12:     DestroyWindow(hWnd);break;
      case VK_LEFT:    JoyState&=~BTN_LEFT;break;
      case VK_RIGHT:   JoyState&=~BTN_RIGHT;break;
      case VK_UP:      JoyState&=~BTN_UP;break;
      case VK_DOWN:    JoyState&=~BTN_DOWN;break;
      case VK_RETURN:  JoyState&=~BTN_START;break;
      case VK_ESCAPE:  JoyState&=~BTN_EXIT;break;
      case VK_TAB:     JoyState&=~BTN_SELECT;break;

      case 'Q': case 'E': case 'T': case 'U': case 'O':
        JoyState&=~BTN_FIREL;break;
      case 'W': case 'R': case 'Y': case 'I': case 'P':
        JoyState&=~BTN_FIRER;break;
      case 'A': case 'S': case 'D': case 'F': case 'G':
      case 'H': case 'J': case 'K': case 'L':
      case VK_SPACE:
        JoyState&=~BTN_FIREA;break;
      case 'Z': case 'X': case 'C': case 'V': case 'B':
      case 'N': case 'M':
        JoyState&=~BTN_FIREB;break;

      default:
        return(0);
    }
  }

  /* Done */
  return(1);
}

/** TimerProc() **********************************************/
/** Synchronization timer callback function.                **/
/*************************************************************/
static void CALLBACK TimerProc(UINT TimerID,UINT Msg,DWORD dwUser,DWORD dw1,DWORD dw2)  
{ 
  TimerReady=1;
} 

/** PlayBuffers() ********************************************/
/** This internal function plays ready audio buffers.       **/
/*************************************************************/
static void PlayBuffers(void)
{
  sample *P;
  int J;

  /* Check if sound is initialized */
  if(!hWave||!SndBuffer) return;

  /* Buffer size and start */
  J=SndSize/SND_HEADERS;
  P=SndBuffer+CurHdr*J;

  /* Until we hit prepared header... */
  while(!(SndHeader[CurHdr].dwFlags&WHDR_PREPARED))
  {
    // Prepare new header
    memset(&SndHeader[CurHdr],0,sizeof(WAVEHDR));
    SndHeader[CurHdr].lpData         = (char *)P;
    SndHeader[CurHdr].dwBufferLength = J*sizeof(sample);
    SndHeader[CurHdr].dwUser         = CurHdr;
    SndHeader[CurHdr].dwFlags        = 0;
    SndHeader[CurHdr].dwLoops        = 0;
    // Enqueue buffer for playback
    if(waveOutPrepareHeader(hWave,&SndHeader[CurHdr],sizeof(WAVEHDR))!=MMSYSERR_NOERROR)
      break;
    if(waveOutWrite(hWave,&SndHeader[CurHdr],sizeof(WAVEHDR))!=MMSYSERR_NOERROR)
      break;
    // Next buffer
    CurHdr=CurHdr<SND_HEADERS-1? CurHdr+1:0;
    P=SndBuffer+CurHdr*J;
  }
}

/** ThreadFunc() *********************************************/
/** This is a thread responsible for receiving messages     **/
/** from waveOut.                                           **/
/*************************************************************/
static DWORD WINAPI ThreadFunc(void *Param)
{
  MSG Msg;
  int I;

  // Get packet-done messages 
  while(GetMessage(&Msg,0,0,0)>0)
    switch(Msg.message)
    {
      case MM_WOM_CLOSE:
        return(0);

      case MM_WOM_DONE:
        if(hWave&&SndBuffer)
        {
          /* Buffer number */
          I=((WAVEHDR *)Msg.lParam-&SndHeader[0]);
          /* Clear buffer */
          memset(SndBuffer+I*SndSize/SND_HEADERS,0,sizeof(sample)*SndSize/SND_HEADERS);
          /* Unprepare header */
          waveOutUnprepareHeader(hWave,(WAVEHDR *)Msg.lParam,sizeof(WAVEHDR));
          /* Move read pointer */
          SndRPtr=SndBuffer+(I<SND_HEADERS-1? I+1:0)*(SndSize/SND_HEADERS);
          /* Enqueue new buffers */
          PlayBuffers();
        }
        break;
    }

  return(0);
}

/** DDInit() *************************************************/
/** Initialize DirectDraw for window W, creating buffer of  **/
/** size WidthxHeight. When one of dimensions is <=0, set   **/
/** up full-screen mode (320x240x16BPP).                    **/
/*************************************************************/
static int DDInit(HWND W,int Width,int Height)
{
  DDSURFACEDESC2 DDSD;

  DDTrash();
  DDFullScreen = (Width==MODE_640x480)||(Height==MODE_640x480)? 2
               : (Width==MODE_320x240)||(Height==MODE_320x240)? 1
               : (Width<=0)||(Height<=0)? 2:0;
  DD           = NULL;
  DDClipper    = NULL;
  DDSurface    = NULL;
  DDWindow     = W;
  DDMenu       = GetMenu(W);
  GetWindowRect(W,&DDRect);


#ifdef _MSC_VER
  /* Latest MSVC versions do not support DirectDraw */
  return(0);
#else
  /* Initialize DirectDraw */
  if(DirectDrawCreateEx(NULL,&DD,&IID_IDirectDraw7,NULL)!=DD_OK)
  { MessageBox(NULL,"DirectDrawCreateEx","Error",0);return(0); }
#endif

  if(!DDFullScreen)
  {
    /* Normal cooperative level, windowed application */
    if(IDirectDraw7_SetCooperativeLevel(DD,W,DDSCL_NORMAL)!=DD_OK)
    { MessageBox(NULL,"SetCooperativeLevel","Error",0);DDTrash();return(0); }
    /* Create the primary surface */
    memset(&DDSD,0,sizeof(DDSD));
    DDSD.dwSize            = sizeof(DDSD);
    DDSD.dwFlags           = DDSD_CAPS;
    DDSD.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE;
    if(IDirectDraw7_CreateSurface(DD,&DDSD,&DDSurface,NULL)!=DD_OK)
    { MessageBox(NULL,"CreateSurface","Error",0);DDTrash();return(0); }
    /* Clipper limits drawing to a window */
    if(IDirectDraw7_CreateClipper(DD,0,&DDClipper,NULL)!=DD_OK)
    { MessageBox(NULL,"CreateClipper","Error",0);DDTrash();return(0); }
    /* Attach clipper to our window */
    if(IDirectDrawClipper_SetHWnd(DDClipper,0,W)!=DD_OK)
    { MessageBox(NULL,"SetHWnd","Error",0);DDTrash();return(0); }
    /* Attach clipper to our drawing surface */
    if(IDirectDrawSurface7_SetClipper(DDSurface,DDClipper)!=DD_OK)
    { MessageBox(NULL,"SetClipper","Error",0);DDTrash();return(0); }
  }
  else
  {
    /* Exclusive cooperative level, full screen application */
    if(IDirectDraw7_SetCooperativeLevel(DD,W,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN)!=DD_OK)
    { MessageBox(NULL,"SetCooperativeLevel","Error",0);DDTrash();return(0); }
    /* Display mode is 320x240x16BPP */
    if(IDirectDraw7_SetDisplayMode(DD,DDFullScreen==2? 640:320,DDFullScreen==2? 480:240,16,0,0)!=DD_OK)
    { MessageBox(NULL,"SetDisplayMode","Error",0);DDTrash();return(0); }
    /* Create the primary surface */
    memset(&DDSD,0,sizeof(DDSD));
    DDSD.dwSize            = sizeof(DDSD);
    DDSD.dwFlags           = DDSD_CAPS;
    DDSD.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE;
    if(IDirectDraw7_CreateSurface(DD,&DDSD,&DDSurface,NULL)!=DD_OK)
    { MessageBox(NULL,"CreateSurface","Error",0);DDTrash();return(0); }
    /* Remove title, border, menu, etc. */ 
    SetWindowLong(W,GWL_STYLE,WS_VISIBLE|WS_POPUP);
    SetWindowPos(W,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);  
    /* Remove cursor */
    ShowCursor(FALSE);
  }

  /* Done */
  return(1);
}

/** DDTrash() ************************************************/
/** Shut down DirectDraw. Return attached window to its     **/
/** previous position and dimensions.                       **/
/*************************************************************/
static void DDTrash()
{
  /* If were running in full screen mode... */
  if(DDFullScreen&&DDWindow&&DD)
  {
    /* Restore display mode and cooperative level */
    IDirectDraw7_RestoreDisplayMode(DD);
    IDirectDraw7_SetCooperativeLevel(DD,DDWindow,DDSCL_NORMAL);
  }

  /* Release resources */
  if(DDSurface) { IDirectDrawSurface7_Release(DDSurface);DDSurface=NULL; }
  if(DDClipper) { IDirectDrawClipper_Release(DDClipper);DDClipper=NULL; }
  if(DD)        { IDirectDraw7_Release(DD);DD=NULL; }

  /* Reset window position, restore border, menu, etc. */
  if(DDFullScreen)
  {
    SetWindowLong(DDWindow,GWL_STYLE,WS_VISIBLE|WS_OVERLAPPEDWINDOW);
    SetWindowPos(
      DDWindow,HWND_NOTOPMOST,
      DDRect.left,DDRect.top,
      DDRect.right-DDRect.left,DDRect.bottom-DDRect.top,
      SWP_FRAMECHANGED
    );  
    SetMenu(DDWindow,DDMenu);
    ShowCursor(TRUE);
  }

  /* DirectDraw is now gone */
  DDFullScreen = 0;
  DDWindow     = NULL;
  DDMenu       = NULL;
}

/** SetSoundSettings() ***************************************/
/** Set sound driver parameters and corresponding menus.    **/
/*************************************************************/
static void SetSoundSettings(int Rate,int Delay)
{
  static int SndRates[]  = { 0,1,11025,22050,44100,-1 };
  static int SndDelays[] = { 10,50,100,250,500,-1 };
  int J;

  ISRate=InitSound(ISRate=Rate,ISDelay=Delay);

  if(hWnd)
  {
    for(J=0;(SndRates[J]>=0)&&(SndRates[J]!=Rate);++J);
    if(SndRates[J]>=0)
      CheckMenuRadioItem(GetMenu(hWnd),WMENU_SNDOFF,WMENU_SND44kHz,WMENU_SNDOFF+J,MF_BYCOMMAND);
    for(J=0;(SndDelays[J]>=0)&&(SndDelays[J]!=Delay);++J);
    if(SndDelays[J]>=0)
      CheckMenuRadioItem(GetMenu(hWnd),WMENU_SND10ms,WMENU_SND500ms,WMENU_SND10ms+J,MF_BYCOMMAND);
  }
}

/** WinSetJoysticks() ****************************************/
/** Shows a dialog to set joystick button mappings.          */
/*************************************************************/
void WinSetJoysticks(unsigned int Buttons)
{
  static const char *ButtonNames[16] =
  {
    "LEFT","RIGHT","UP","DOWN","FIRE-A","FIRE-B","FIRE-L","FIRE-R",
    "START","SELECT","EXIT","?","?","?","?","?"
  };
  JOYINFOEX JS;
  char S[256];
  int I,J;

  /* Have to have a joystick! */
  if(!JoyCount)
  {
    MessageBox(hWnd,
      "There is no joystick connected to this computer.\n"
      "Please, connect a joystick first!",
      "Configure Joystick",
      MB_ICONERROR|MB_OK
    );
    return;
  }

  /* Only setting fire buttons */
  J=BTN_FIREA|BTN_FIREB|BTN_FIREL|BTN_FIRER|BTN_START|BTN_SELECT|BTN_EXIT;
  Buttons&=J|(J<<16);

  /* Check joystick(s) */
  JS.dwSize  = sizeof(JS);
  JS.dwFlags = JOY_RETURNBUTTONS;
  for(I=0;I<JoyCount;I++,Buttons>>=16)
    for(J=0;J<16;++J)
      if(Buttons&(1<<J))
      {
        /* Compose message */
        sprintf(
          S,
         "Please, click OK while holding buttons corresponding\n"
         "to %s on joystick #%d. Click CANCEL to abort.",
         ButtonNames[J],I+1
        );
        /* Show dialog box */
        if(MessageBox(hWnd,S,"Configure Joystick",MB_ICONQUESTION|MB_OKCANCEL))
          /* If joystick state acquisition successful, set mask */
          if((joyGetPosEx(JOYSTICKID1+I,&JS)==JOYERR_NOERROR)&&JS.dwButtons)
            switch(1<<J)
            {
              case BTN_FIREA:  JoyButtons[I][0]=JS.dwButtons;break;
              case BTN_FIREB:  JoyButtons[I][1]=JS.dwButtons;break;
              case BTN_FIREL:  JoyButtons[I][2]=JS.dwButtons;break;
              case BTN_FIRER:  JoyButtons[I][3]=JS.dwButtons;break;
              case BTN_START:  JoyButtons[I][4]=JS.dwButtons;break;
              case BTN_SELECT: JoyButtons[I][5]=JS.dwButtons;break;
              case BTN_EXIT:   JoyButtons[I][6]=JS.dwButtons;break;
            }
      }
}
