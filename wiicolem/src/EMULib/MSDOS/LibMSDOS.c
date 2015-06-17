/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       LibMSDOS.c                        **/
/**                                                         **/
/** This file contains MSDOS-dependent implementation       **/
/** parts of the emulation library.                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "EMULib.h"
#include "Sound.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <direct.h>
#include <dos.h>
#include <i86.h>

#define IRQ0_NEWFREQ  (1193182L/8192L)    /* New IRQ0 freq.  */

extern int MasterSwitch; /* Switches to turn channels on/off */
extern int MasterVolume; /* Master volume                    */

static int VGAMode    = 0; /* Current screen mode            */
static int MouseON    = 0; /* 1: Mouse initialized           */
static int SyncFreq   = 0; /* Sync timer frequency, Hz       */
static int Effects    = 0; /* EFF_* bits, ORed               */
static char PalBuf[768];   /* Storage for VGA palette        */
static Image OutScreen;    /* Special effects output image   */

static volatile int SyncReady = 0;    /* 1: Sync timer ready */
static volatile unsigned int JoyState = 0; /* Joystick state */
static volatile unsigned int LastKey  = 0; /* Last key prsd  */
static volatile unsigned int KeyModes = 0; /* SHIFT/CTRL/ALT */

static int SndRate    = 0; /* Audio sampling rate            */
static int SBPort;         /* SoundBlaster port address      */
static int SBIRQ;          /* SoundBlaster IRQ               */
static int SBDMA;          /* SoundBlaster DMA channel       */
static int SBBuffers;      /* Total number of SB buffers     */
static volatile int SBRead;/* Currently read SB buffer       */
static int SBWrite;        /* Currently written SB buffer    */
static unsigned char *SBBuffer; /* SB buffers in DOS memory  */
static unsigned short SBSegment=0;  /* SBBuffer segment      */
static volatile int SBIRQBusy; /* IRQ5Handler() is operating */

static void (interrupt *IRQ0Old)() = 0;    /* Old timer IRQ  */
static void (interrupt *IRQ1Old)() = 0;    /* Old kbrd IRQ   */
static void (interrupt *IRQ5Old)() = 0;    /* Old audio IRQ  */

static void interrupt IRQ0Handler(void);
static void interrupt IRQ1Handler(void);
static void interrupt IRQ5Handler(void);

static const unsigned char KeyCodes[256] =
{
  0,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=',0x08,0x09,
  'q','w','e','r','t','y','u','i','o','p','[',']',0x0D,0,'a','s',
  'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
  'b','n','m',',','.','/',0,0,0,' ',0,CON_F1,CON_F2,CON_F3,CON_F4,CON_F5,
  CON_F6,CON_F7,CON_F8,CON_F9,CON_F10,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,CON_F11,CON_F12,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,CON_UP,0,0,CON_LEFT,0,CON_RIGHT,0,0,
  CON_DOWN,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static int ReadDSP(void);          /* Read value from DSP    */
static int WriteDSP(int V);        /* Write value into DSP   */
static void WriteMixer(int R,int V); /* Write V into mixer   */
static void Lock(void);            /* Set internal lock      */
static void Unlock(void);          /* Release internal lock  */
static unsigned short GetSegment(unsigned short Paragraphs);
static void FreeSegment(unsigned short Segment);

int VGAPutImageWH(const Image *Img,int XD,int YD,int XS,int YS,int W,int H);

/** InitMSDOS() **********************************************/
/** Initialize MSDOS keyboard handler, etc.                 **/
/*************************************************************/
void InitMSDOS(void)
{
#ifdef BPP8
  int J;
  /* Create RRR:GGG:BB static color palette in BPP8 mode */
  for(J=0;J<256;J++)
    SetPalette(J,(J>>5)*255/7,((J>>2)&0x07)*255/7,(J&0x03)*255/3);
#endif

  /* Initialize variables */
  IRQ1Old   = _dos_getvect(0x09);
  IRQ0Old   = 0;
  MouseON   = 0;
  SyncReady = 0;
  SyncFreq  = 0;
  VGAMode   = 0;
  JoyState  = 0;
  LastKey   = 0;
  KeyModes  = 0;
  SndRate   = 0;

  /* No output image yet */
  OutScreen.Data = 0;

  /* Install new keyboard interrupt */
  _dos_setvect(0x09,IRQ1Handler);
}

/** TrashMSDOS() *********************************************/
/** Restore MSDOS keyboard handler, etc.                    **/
/*************************************************************/
void TrashMSDOS(void)
{
  /* Restore keyboard IRQ */
  if(IRQ1Old) { _dos_setvect(0x09,IRQ1Old);IRQ1Old=0; }

  /* Restore timer IRQ */
  SetSyncTimer(0);

  /* Shut down audio */
  TrashAudio();

  /* Set text mode 80x25 */
  VGAText80();

  /* Free effect buffer */
  FreeImage(&OutScreen);
}

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency)
{
  static const int PagePort[8] = { 0x87,0x83,0x81,0x82,-1,0x8B,0x89,0x8A };
  int J,TotalBuf;
  char *P;

  /* Disable audio if was initialized */
  TrashAudio();

  /* If turning audio off, drop out */
  if(!Rate) return(1);

  /* If Rate is out of range, fall out */
  if((Rate<8192)||(Rate>44100)) return(0);

  /* Default values */
  SBPort    = 0x220;
  SBIRQ     = 7;
  SBDMA     = 1;
  SBIRQBusy = 0;
  SBWrite   = 0;
  SBRead    = 0;

  /* Get SoundBlaster Port#,IRQ#,DMA# */
  if(P=getenv("BLASTER"))
    while(*P)
      switch(toupper(*P++))
      {
        case 'A': SBPort=strtol(P,0,16);break;
        case 'D': SBDMA=strtol(P,0,16);break;
        case 'I': SBIRQ=strtol(P,0,16);break;
      }

  /* IRQ# = 0,1,2,3,4,5,6,7, DMA# = 0,1,2,3 */
  if((SBIRQ>7)||(SBDMA>3)) return(0);

  /* Reset DSP */
  outp(SBPort+0x06,0x01); /* Set the reset flag   */
  delay(100);             /* Wait for 100ms       */
  outp(SBPort+0x06,0x00); /* Clear the reset flag */

  /* Wait for READY status */
  for(J=100;J&&!(inp(SBPort+0x0E)&0x80);J--);
  if(!J) return(0);
  for(J=100;J&&(ReadDSP()!=0xAA);J--);
  if(!J) return(0);

  /* Allocate low memory for DMA buffer */
  TotalBuf=(Rate*Latency/1000/SND_BUFSIZE)*SND_BUFSIZE;
  if(TotalBuf<SND_BUFSIZE) TotalBuf=SND_BUFSIZE;
  SBBuffers=TotalBuf/SND_BUFSIZE;
  SBSegment=GetSegment(TotalBuf*sizeof(sample)*2/16+4);
  if(!SBSegment) return(0);
  J=SBSegment*16;
  SBBuffer=(unsigned char *)J;
  if((J>>16)!=((J+TotalBuf-1)>>16)) SBBuffer+=TotalBuf;
  memset(SBBuffer,0,TotalBuf*sizeof(sample));

  /* Interrupts off */
  _disable();

  /* Save old interrupt vector */
  IRQ5Old=(void (interrupt *)())_dos_getvect(SBIRQ+8);
  /* Install new interrupt vector */
  _dos_setvect(SBIRQ+8,IRQ5Handler);
  /* Enable the SoundBlaster IRQ */
  outp(0x21,inp(0x21)&~(1<<SBIRQ));

  /* Disable DMA channel */
  outp(0x0A,SBDMA|0x04);
  /* Clear byte pointer flip-flop */
  outp(0x0C,0x00);
  /* Auto-initialized playback mode */
  outp(0x0B,SBDMA|0x58);
  /* Write DMA offset and transfer length */
  J=SBDMA<<1;
  outp(J,(int)SBBuffer&0xFF);
  outp(J,((int)SBBuffer>>8)&0xFF);
  outp(J+1,(TotalBuf-1)&0xFF);
  outp(J+1,((TotalBuf-1)>>8)&0xFF);
  /* Write DMA page */
  outp(PagePort[SBDMA],((int)SBBuffer>>16)&0xFF);
  /* Enable DMA channel */
  outp(0x0A,SBDMA);

  /* Enable interrupts */
  _enable();

  /* Set sampling rate */
  WriteDSP(0x40);
  WriteDSP((65536L-(256000000L/Rate))/256);
  /* Set DMA transfer size */
  WriteDSP(0x48);
  WriteDSP((SND_BUFSIZE-1)&0xFF);
  WriteDSP((SND_BUFSIZE-1)>>8);
  /* Run auto-initialized 8bit DMA */
  WriteDSP(0x90);
  /* Speaker enabled */
  WriteDSP(0xD1);

  /* Done */
  return(SndRate=Rate);
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
  /* If SoundBlaster was initialized... */
  if(SndRate)
  {
    /* Reset DSP */
    WriteDSP(0xD3);
    WriteDSP(0xD0);
    /* Disable DMA */
    outp(0x0A,SBDMA|0x04);
    /* Disable IRQ */
    outp(0x21,inp(0x21)|(1<<SBIRQ));
  }

  /* If SB interrupt vector changed, restore it */
  if(IRQ5Old) { _dos_setvect(SBIRQ+8,IRQ5Old);IRQ5Old=0; }
  /* If low memory was allocated for DMA, free it */
  if(SBSegment) { FreeSegment(SBSegment);SBSegment=0; }
  /* Sound trashed */
  SndRate=0;
}

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void)
{
  int SW,SH,J;

  /* Must have active video image */
  if(!VideoImg||!VideoImg->Data) return(0);

  switch(VGAMode)
  {
    case 1:  SW=320;SH=200;break;
    case 2:  SW=256;SH=240;break;
    case 3:  SW=800;SH=600;break;
    case 4:  SW=640;SH=480;break;
    default: return(0);
  }

  /* If applying image effects... */
  if(Effects&(EFF_SOFTEN|EFF_SCALE|EFF_TVLINES))
  {
    /* Allocate effect buffer if needed */
    if(!OutScreen.Data||(OutScreen.W!=SW)||(OutScreen.H!=SH))
    { FreeImage(&OutScreen);NewImage(&OutScreen,SW,SH); }

    /* If effect buffer present... */
    if(OutScreen.Data)
    {
      Image Img;
      int W,H;

      /* Apply effects */
      if(Effects&EFF_SOFTEN)
      {
        /* If video is squished, unsquish it */
        H = VideoW/VideoH;
        H = H>1? VideoH*H:VideoH;
        W = VideoH/VideoW;
        W = W>1? VideoW*W:VideoW;
        /* Try filling the screen */
        if(W*SH/H>SW) { H=H*SW/W;W=SW; } else { W=W*SH/H;H=SH; }
        /* Crop work region with the right W/H ratio */
        CropImage(&Img,&OutScreen,(SW-W)>>1,(SH-H)>>1,W,H);
        /* Scale VideoImg with softening algorithm */
        SoftenImage(&Img,VideoImg,VideoX,VideoY,VideoW,VideoH);
      }
      else if(Effects&EFF_SCALE)
      {
        /* If video is squished, unsquish it */
        H = VideoW/VideoH;
        H = H>1? VideoH*H:VideoH;
        W = VideoH/VideoW;
        W = W>1? VideoW*W:VideoW;
        /* Try filling the screen */
        J = SW/W<SH/H? SW/W:SH/H;
        W*= J;
        H*= J;
        /* Crop work region with the right W/H ratio */
        CropImage(&Img,&OutScreen,(SW-W)>>1,(SH-H)>>1,W,H);
        /* Scale VideoImg dumbly */
        ScaleImage(&Img,VideoImg,VideoX,VideoY,VideoW,VideoH);
      }
      else
      {
        /* Just center VideoImg in OutScreen */
        W = SW>VideoW? VideoW:SW;
        H = SH>VideoH? VideoH:SH;
        CropImage(&Img,&OutScreen,(SW-W)>>1,(SH-H)>>1,W,H);
        IMGCopy(
          &Img,0,0,VideoImg,
          VideoX+(SW>VideoW? 0:((VideoW-SW)>>1)),
          VideoY+(SH>VideoH? 0:((VideoH-SH)>>1)),
          W,H,-1
        );
      }

      /* If needed, simulate TV scanlines */
      if(Effects&EFF_TVLINES) TelevizeImage(&Img,0,0,Img.W,Img.H);
      /* If needed, sync to timer */
      if(Effects&EFF_SYNC) WaitSyncTimer();
      /* If needed, sync to VBlank */
      if(Effects&EFF_VSYNC) VGAWaitVBlank();
      /* Call VGAPutImageWH() */
      return(VGAPutImageWH(&OutScreen,0,0,0,0,SW,SH));
    }
  }

  /* If needed, sync to timer */
  if(Effects&EFF_SYNC) WaitSyncTimer();
  /* If needed, sync to VBlank */
  if(Effects&EFF_VSYNC) VGAWaitVBlank();

  /* Call VGAPutImageWH() */
  return(VGAPutImageWH(
    VideoImg,
    (SW-VideoW)/2,(SH-VideoH)/2,
    VideoX,VideoY,
    VideoW,VideoH
  ));
}

/** SetPalette() *********************************************/
/** Set color N to the given <R,G,B> value. This only works **/
/** for palette-based modes.                                **/
/*************************************************************/
void SetPalette(pixel N,unsigned char R,unsigned char G,unsigned char B)
{
  /* Set VGA palette entry to a given value */
  outp(0x3C8,N);
  outp(0x3C9,PalBuf[N*3]=63*R/255);
  outp(0x3C9,PalBuf[N*3+1]=63*G/255);
  outp(0x3C9,PalBuf[N*3+2]=63*B/255);
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{
  /* Return distance between read and write indices */
  return(
    !SndRate?        0
  : SBRead>=SBWrite? SBRead-SBWrite
  :                  SBBuffers*SND_BUFSIZE-SBWrite+SBRead-1
  );

//  unsigned int J;
  /* No sound, no samples */
//  if(!SndRate) return(0);
  /* Get current DMA remainder */
//  J  = inp((SBDMA<<1)+1);
//  J += inp((SBDMA<<1)+1)<<8;
  /* Convert to read index */
//  J  = SBBuffers*SND_BUFSIZE-J-1;
  /* Return distance between read and write indices */
//  return(J>=SBWrite? J-SBWrite:SBBuffers*SND_BUFSIZE-SBWrite+J-1);
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
  unsigned int J;

  /* Truncate Length if needed */
  J      = GetFreeAudio();
  Length = J<Length? J:Length;

  /* Copy samples to the DMA buffer */
  for(J=Length;J;--J,++Data)
  {
    SBBuffer[SBWrite]=*Data+128;
    SBWrite=(SBWrite+1)%(SBBuffers*SND_BUFSIZE);
  }

  /* Return number of samples copied */
  return(Length);
}

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings.             **/
/*************************************************************/
unsigned int GetJoystick(void)
{
  unsigned int J;

  /* Combine joystick state */
  J = JoyState|KeyModes;
  /* Swap players #1/#2 when ALT button pressed */
  if(J&BTN_ALT) J=(J&BTN_MODES)|((J>>16)&BTN_ALL)|((J&BTN_ALL)<<16);
  /* Done */
  return(J);
}

/** GetMouse() ***********************************************/
/** Get mouse position and button states in the following   **/
/** format: RMB.LMB.Y[29-16].X[15-0].                       **/
/*************************************************************/
unsigned int GetMouse(void)
{
  union REGS R;
  int J;

  /* Only 320x200 and 256x240 screen modes are allowed */
//  if((VGAMode!=1)&&(VGAMode!=2)) return(0);
  /* No mouse in text mode */
  if(!VGAMode) return(0);

  /* Initialize mouse if needed */
  if(!MouseON)
  {
    R.w.ax=0x0000;
    int386(0x33,&R,&R);
    if(!R.w.ax) return(0);
    else
    {
      /* Mouse is now on */
      MouseON=1;
      /* Turn mouse pointer off */
      R.w.ax = 0x0002;
      int386(0x33,&R,&R);
      /* Set horizontal range */
      R.w.ax = 0x0007;
      R.w.cx = 0;
      R.w.dx = VGAMode==4? 640:VGAMode==3? 800:VGAMode==2? 256:320;
      int386(0x33,&R,&R);
      /* Set vertical range */
      R.w.ax = 0x0008;
      R.w.cx = 0;
      R.w.dx = VGAMode==4? 480:VGAMode==3? 600:VGAMode==2? 240:200;
      int386(0x33,&R,&R);
    }
  }

  /* Get mouse state */
  R.w.ax = 0x0003;
  int386(0x33,&R,&R);

  /* Done */
  return(R.w.cx|((int)R.w.dx<<16)|((int)R.w.bx<<30));
}

/** GetKey() *************************************************/
/** Get currently pressed key or 0 if none pressed. Returns **/
/** CON_* definitions for arrows and special keys.          **/
/*************************************************************/
unsigned int GetKey(void)
{
  unsigned int J;

  if(IRQ1Old)
  {
    _disable();
    J=LastKey;
    LastKey=0;
    _enable();
    return(J);
  }
  else
  {
    J=kbhit();
    return(!J||(J==0xE0)? KeyCodes[0x80|kbhit()]:J);
  }
}

/** WaitKey() ************************************************/
/** Wait for a key to be pressed. Returns CON_* definitions **/
/** for arrows and special keys.                            **/
/*************************************************************/
unsigned int WaitKey(void)
{
  if(IRQ1Old)
  {
    while(!LastKey&&VideoImg);
    return(GetKey());
  }
  else
  {
    unsigned int J;
    J=getch();
    return(!J||(J==0xE0)? KeyCodes[0x80|getch()]:J);
  }
}

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready.                     **/
/*************************************************************/
void WaitSyncTimer(void)
{
  /* If no IRQ0 handler, return immediately */
  if(!IRQ0Old) return;
  /* Wait for sync timer to go up */
  while(!SyncReady) {}
  /* Reset sync timer */
  SyncReady=0;
}

/** SyncTimerReady() *****************************************/
/** Return 1 if sync timer ready, 0 otherwise.              **/
/*************************************************************/
int SyncTimerReady(void)
{
  /* Return sync timer status */
  return(!IRQ0Old||SyncReady);
}

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz)
{
  unsigned int J;

  /* Remove sync timer if requested */
  if(!Hz)
  {
    if(IRQ0Old)
    {
      /* Restore the timer */
      _disable();
      outp(0x43,0x36);
      outp(0x40,0x00);
      outp(0x40,0x00);
      _enable();
      /* Restore timer IRQ  */
      _dos_setvect(0x08,IRQ0Old);
      IRQ0Old=0;
    }
    SyncFreq=0;
    return(1);
  }

  /* Check for valid values of 20..100Hz */
  if((Hz<20)||(Hz>100)) return(0);

  /* Set sync frequency */
  SyncFreq=Hz;

  /* Install new timer interrupt */
  if(!IRQ0Old)
  {
    IRQ0Old=_dos_getvect(0x08);
    _dos_setvect(0x08,IRQ0Handler);
  }

  /* Set timer to the base sync frequency */
  J=1193182L/IRQ0_NEWFREQ;
  _disable();
  outp(0x43,0x36);
  outp(0x40,J&0xFF);
  outp(0x40,J>>8);
  _enable();

  /* Done */
  return(1);
}

/** ChangeDir() **********************************************/
/** This function is a wrapper for chdir(). Unlike chdir(), **/
/** it changes *both* the drive and the directory in MSDOS, **/
/** exactly like the "normal" chdir() should. Returns 0 on  **/
/** success, -1 on failure, just like chdir().              **/
/*************************************************************/
int ChangeDir(const char *Name)
{
  unsigned int J,K;

  /* Change directory */
  if(chdir(Name)<0) return(-1);

  /* If drive name present... */
  if((Name[1]==':')&&isalpha(Name[0]))
  {
    /* Extract drive name */
    if((Name[0]>='a')&&(Name[0]<='z')) J=Name[0]-'a'+1;
    else if((Name[0]>='A')&&(Name[0]<='Z')) J=Name[0]-'A'+1;
         else return(-1);
    /* Set drive and check if it has been set */
    _dos_setdrive(J,&K);
    _dos_getdrive(&K);
    if(J!=K) return(-1);
  }

  /* Done */
  return(0);
}

/** VGASetEffects() ******************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void VGASetEffects(int NewEffects)
{
  /* Set new effects, free effect buffer if no longer needed */
  if(!(Effects=NewEffects)) FreeImage(&OutScreen);
}

/** VGAWaitVBlank() ******************************************/
/** Wait VGA VBlank.                                        **/
/*************************************************************/
void VGAWaitVBlank(void)
{
// @@@ Takes too long
//  while(inp(0x3DA)&0x08);
  while(!(inp(0x3DA)&0x08));
}

/** VGAText80() **********************************************/
/** Set 80x25 text screenmode.                              **/
/*************************************************************/
void VGAText80(void)
{
  union REGS R;
  int J;

  /* Save palette */
  if(VGAMode)
    for(J=0,outp(0x3C7,0);J<768;J++)
      PalBuf[J]=inp(0x3C9);

  R.w.ax  = 0x0003;
  int386(0x10,&R,&R);
  VGAMode = 0;
  MouseON = 0;
}

/** VGA320x200() *********************************************/
/** Set 320x200 screenmode, 8bit or 16bit.                  **/
/*************************************************************/
void VGA320x200(void)
{
  union REGS R;
  int J;

#ifdef BPP16
  /* Set 5:5:5 320x200 screen */
  R.w.ax = 0x4F02;
  R.w.bx = 0x010D;
  int386(0x10,&R,&R);
  /* Set scanline length to 512 pixels */
  R.w.ax = 0x4F06;
  R.w.bx = 0x0000;
  R.w.cx = 0x0200;
  int386(0x10,&R,&R);
  /* Clear pages */
  for(J=0;J<4;J++)
  {
    R.w.ax = 0x4F05;
    R.w.bx = 0x0000;
    R.w.dx = J;
    int386(0x10,&R,&R);
    memset((char *)0xA0000,0x00,0x10000);
  }
  /* Set first memory page */
  R.w.ax = 0x4F05;
  R.w.bx = 0x0000;
  R.w.dx = 0x0000;
  int386(0x10,&R,&R);
#else
  /* Set 320x200x256 screen */
  R.w.ax = 0x0013;
  int386(0x10,&R,&R);
  /* Clear screen */
  memset((char *)0xA0000,0,320*200);
  /* Restore palette */
  if(!VGAMode)
    for(J=0,outp(0x3C8,0);J<768;J++)
      outp(0x3C9,PalBuf[J]);
#endif

  VGAMode=1;
  MouseON=0;
}

/** VGA800x600() *********************************************/
/** Set 800x600 screenmode.                                 **/
/*************************************************************/
void VGA800x600(void)
{
  union REGS R;
  int J;

#ifdef BPP16
  /* @@@ 0x0113 for 5:5:5 */
  /* @@@ 0x0114 for 5:6:5 */
  VESASetMode(0x0113);
#else
  VESASetMode(0x0103);
#endif

  /* Set scanline length to 1024 pixels */
  R.w.ax = 0x4F06;
  R.w.bx = 0x0000;
  R.w.cx = 0x0400;
  int386(0x10,&R,&R);

  VGAMode=3;
  MouseON=0;
}

/** VGA640x480() *********************************************/
/** Set 640x480 screenmode.                                 **/
/*************************************************************/
void VGA640x480(void)
{
  union REGS R;
  int J;

#ifdef BPP16
  VESASetMode(0x0110);
#else
  VESASetMode(0x0101);
#endif

  /* Set scanline length to 1024 pixels */
  R.w.ax = 0x4F06;
  R.w.bx = 0x0000;
  R.w.cx = 0x0400;
  int386(0x10,&R,&R);

  VGAMode=4;
  MouseON=0;
}

/** VGA256x240() *********************************************/
/** Set 256x240x256 screenmode.                             **/
/*************************************************************/
void VGA256x240(void)
{
  static const unsigned char Regs[24] =
  {
    0x4F,0x3F,0x40,0x92,
    0x44,0x10,0x0A,0x3E,
    0x00,0x41,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0xEA,0xAC,0xDF,0x20,
    0x40,0xE7,0x06,0xA3
  };
  union REGS R;
  int J;

  /* Set 320x200x256 first */
  R.w.ax=0x0013;
  int386(0x10,&R,&R);

  _disable();                /* Interrupts off        */
  VGAWaitVBlank();           /* Wait for VBlank       */
  outpw(0x3C4,0x100);        /* Sequencer reset       */
  outp(0x3C2,0xE3);          /* Misc. output register */
  outpw(0x3C4,0x300);        /* Clear sequencer reset */

  /* Unprotect registers 0-7 */
  outpw(0x3D4,((Regs[0x11]&0x7F)<<8)+0x11);
  for(J=0;J<24;J++) outpw(0x3D4,(Regs[J]<<8)+J);

  _enable();

  /* Clear screen */
  memset((char *)0xA0000,0,256*240);
  /* Restore palette */
  if(!VGAMode)
    for(J=0,outp(0x3C8,0);J<768;J++)
      outp(0x3C9,PalBuf[J]);

  VGAMode=2;
  MouseON=0;
}

/** VESASetMode() ********************************************/
/** Set VESA screen mode.                                   **/
/*************************************************************/
void VESASetMode(int Mode)
{
  union REGS R;
  int J;

  /* Set requested screen mode */
  R.w.ax = 0x4F02;
  R.w.bx = Mode;
  int386(0x10,&R,&R);

  /* Clear pages */
  for(J=0;J<16;J++)
  {
    R.w.ax = 0x4F05;
    R.w.bx = 0x0000;
    R.w.dx = J;
    int386(0x10,&R,&R);
    memset((char *)0xA0000,0x00,0x10000);
  }

  /* Set first memory page */
  R.w.ax = 0x4F05;
  R.w.bx = 0x0000;
  R.w.dx = 0x0000;
  int386(0x10,&R,&R);

#ifdef BPP8
  /* Restore palette */
  if(!VGAMode)
    for(J=0,outp(0x3C8,0);J<768;J++)
      outp(0x3C9,PalBuf[J]);
#endif
}

/** VGAPutImageWH() ******************************************/
/** Put an image on the screen at given coordinates.        **/
/*************************************************************/
int VGAPutImageWH(const Image *Img,int XD,int YD,int XS,int YS,int W,int H)
{
  union REGS R;
  pixel *D,*S;
  int J;

  /* If incorrect parameters, fall out */
  if((XS+W>Img->W)||(YS+H>Img->H)) return(0);

  switch(VGAMode)
  {
    case 4: /* For 640x480 mode... */
    case 3: /* For 800x600 mode... */
      /* Clip image if bigger than screen */
      J = VGAMode==4? 640:800;
      W = W>=J? J-XD:W;
      J = VGAMode==4? 480:600;
      H = H>=J? J-YD:H;
      /* Calculate source address */
      S=Img->Data+Img->W*YS+XS;
      /* Calculate destination and copy image */
      J=1024*YD+XD;
      D=(pixel *)0xA0000+(J&0x7FFF);
      for(J>>=15;H;J++,D-=0x8000)
      {
        /* Set memory page */
        R.w.ax = 0x4F05;
        R.w.bx = 0x0000;
        R.w.dx = J;
        int386(0x10,&R,&R);
        /* Copy image */
        for(;H&&(D<(pixel *)0xB0000);H--,D+=1024,S+=Img->W)
          memcpy(D,S,W*sizeof(pixel));
      }
      /* Set first memory page */
      R.w.ax = 0x4F05;
      R.w.bx = 0x0000;
      R.w.dx = 0x0000;
      int386(0x10,&R,&R);
      break;

    case 2: /* For 256x240 mode... */
      /* Clip image if bigger than screen */
      if(W>=256) W=256-XD;
      if(H>=240) H=240-YD;
      /* Calculate source/destination */
      D=(pixel *)0xA0000+256*YD+XD;
      S=Img->Data+Img->W*YS+XS;
      /* Copy image */
      for(;H;H--,D+=256,S+=Img->W) memcpy(D,S,W*sizeof(pixel));
      break;

    case 1: /* For 320x200 mode... */
      /* Clip image if bigger than screen */
      if(W>=320) W=320-XD;
      if(H>=200) H=200-YD;
      /* Calculate source address */
      S=Img->Data+Img->W*YS+XS;
      /* Calculate destination and copy image */
#ifdef BPP16
      J=512*YD+XD;
      D=(pixel *)0xA0000+(J&0x7FFF);
      for(J>>=15;H;J++,D-=0x8000)
      {
        /* Set memory page */
        R.w.ax = 0x4F05;
        R.w.bx = 0x0000;
        R.w.dx = J;
        int386(0x10,&R,&R);
        /* Copy image */
        for(;H&&(D<(pixel *)0xB0000);H--,D+=512,S+=Img->W)
          memcpy(D,S,W*sizeof(pixel));
      }
      /* Set first memory page */
      R.w.ax = 0x4F05;
      R.w.bx = 0x0000;
      R.w.dx = 0x0000;
      int386(0x10,&R,&R);
#else
      D=(pixel *)0xA0000+320*YD+XD;
      for(;H;H--,D+=320,S+=Img->W)
        memcpy(D,S,W);
#endif
      break;

    default: /* Unsupported screen mode */
      return(0);
  }

  /* Done */
  return(1);
}

/** IRQ0Handler() ********************************************/
/** A handler for the timer interrupt (not currently used). **/
/*************************************************************/
static void interrupt IRQ0Handler(void)
{
  static int IRQ0Count = 0;
  static int SyncCount = 0;

  /* Count time for the screen sync */
  if(--SyncCount<=0)
  {
    /* Reset counter till the next sync */
    SyncCount+=IRQ0_NEWFREQ/SyncFreq;
    /* Sync is ready */
    SyncReady=1;
  }

  /* Count time for the old interrupt handler */
  if(--IRQ0Count<=0)
  {
    /* Reset counter till the next call */
    IRQ0Count+=65536L*IRQ0_NEWFREQ/1193182L;
    /* Call old interrupt handler */
    (*IRQ0Old)();
  }
}

/** IRQ1Handler() ********************************************/
/** A handler for the keyboard interrupt.                   **/
/*************************************************************/
static void interrupt IRQ1Handler(void)
{
  static int Special=0;
  register unsigned char Key,J;

  /* Scan and acknowledge key code */
  Key=inp(0x60);
  J=inp(0x61);
  outp(0x61,J|0x80);
  outp(0x61,J&0x7F);

  /* Process keycodes */
  if(Special)
    switch(Key)
    {
      case 0x48: JoyState|=BTN_UP;break;
      case 0x50: JoyState|=BTN_DOWN;break;
      case 0x4D: JoyState|=BTN_RIGHT;break;
      case 0x4B: JoyState|=BTN_LEFT;break;
      case 0xC8: JoyState&=~BTN_UP;break;
      case 0xD0: JoyState&=~BTN_DOWN;break;
      case 0xCD: JoyState&=~BTN_RIGHT;break;
      case 0xCB: JoyState&=~BTN_LEFT;break;
      case 0x49: /* PAGE-UP */
        if(KeyModes&CON_ALT)
        {
          /* Volume up */
          SetChannels(MasterVolume<247? MasterVolume+8:255,MasterSwitch);
          /* Done with IRQ */
          outp(0x20,0x20);
          return;
        } 
        break;
      case 0x51: /* PAGE-DOWN */
        if(KeyModes&CON_ALT)
        {
          /* Volume down */
          SetChannels(MasterVolume>8? MasterVolume-8:0,MasterSwitch);
          /* Done with IRQ */
          outp(0x20,0x20);
          return;
        } 
        break;
    }
  else
  {
    switch(Key)
    {
      case 0x2A:                                 /* LSHIFT */
      case 0x36: KeyModes|=CON_SHIFT;break;      /* RSHIFT */
      case 0x3A: KeyModes|=CON_CAPS;break;       /* CAPSLK */
      case 0x1D: KeyModes|=CON_CONTROL;break;    /* CTRL   */        
      case 0x38: KeyModes|=CON_ALT;break;        /* ALT    */
      case 0x01: JoyState|=BTN_EXIT;break;       /* ESCAPE */
      case 0x0F: JoyState|=BTN_SELECT;break;     /* TAB    */
      case 0x1C: JoyState|=BTN_START;break;      /* ENTER  */
      case 0xAA:                                 /* LSHIFT */
      case 0xB6: KeyModes&=~CON_SHIFT;break;     /* RSHIFT */
      case 0xBA: KeyModes&=~CON_CAPS;break;      /* CAPSLK */
      case 0x9D: KeyModes&=~CON_CONTROL;break;   /* CTRL   */        
      case 0xB8: KeyModes&=~CON_ALT;break;       /* ALT    */
      case 0x81: JoyState&=~BTN_EXIT;break;      /* ESCAPE */
      case 0x8F: JoyState&=~BTN_SELECT;break;    /* TAB    */
      case 0x9C: JoyState&=~BTN_START;break;     /* ENTER  */

      case 0x10: case 0x12: case 0x14: case 0x16: case 0x18:
        JoyState|=BTN_FIREL;break;        /* Q,E,T,U,O pressed  */
      case 0x90: case 0x92: case 0x94: case 0x96: case 0x98:
        JoyState&=~BTN_FIREL;break;       /* Q,E,T,U,O released */
      case 0x11: case 0x13: case 0x15: case 0x17: case 0x19:
        JoyState|=BTN_FIRER;break;        /* W,R,Y,I,P pressed  */
      case 0x91: case 0x93: case 0x95: case 0x97: case 0x99:
        JoyState&=~BTN_FIRER;break;       /* W,R,Y,I,P pressed  */

      case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22:
      case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
      case 0x28: case 0x29:
      case 0x39:                          /* SPACE pressed  */
        JoyState|=BTN_FIREA;
        break;

      case 0x2C: case 0x2D: case 0x2E: case 0x2F: case 0x30:
      case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
        JoyState|=BTN_FIREB;
        break;

      case 0x9E: case 0x9F: case 0xA0: case 0xA1: case 0xA2:
      case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
      case 0xA8: case 0xA9:
      case 0xB9:                          /* SPACE released */
        JoyState&=~BTN_FIREA;
        break;

      case 0xAC: case 0xAD: case 0xAE: case 0xAF: case 0xB0:
      case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:
        JoyState&=~BTN_FIREB;
        break;
    }
  }

  /* Map key code */
  J=KeyCodes[(Special? 0x80:0x00)|(Key&0x7F)];
  J=(J>='a')&&(J<='z')? J+'A'-'a':J;

  /* Call optional keyboard handler */
  if(KeyHandler)
    (*KeyHandler)(
      (J? J:(Key|0x80|(Special? 0x100:0))) /* Key code                 */
    | KeyModes                             /* CON_SHIFT|ALT|CONTROL    */
    | (((int)Key&0x80)<<24)                /* 0x80000000 on release    */
    );

  /* Process console keypresses */
  if(!(Key&0x80)&&J)
    LastKey=J==0x0D? CON_OK:J==0x1B? CON_EXIT:J;

  /* Detect extended codes */
  Special=!Special&&(Key==0xE0);

  /* Done with IRQ */
  outp(0x20,0x20);
}

/** IRQ5Handler() ********************************************/
/** A handler for the SoundBlaster interrupt.               **/
/*************************************************************/
static void interrupt IRQ5Handler(void)
{
  unsigned int J,I;

  /* Acknowledge interrupt */
  inp(SBPort+0x0E);
  outp(0x20,0x20);
  if(SBIRQ>=8) outp(0xA0,0x20);

  /* Clear played data */
  for(J=SBRead,I=SND_BUFSIZE;I;--I) SBBuffer[J++]=128;

  /* Update the read position */
  SBRead=(SBRead+SND_BUFSIZE)%(SND_BUFSIZE*SBBuffers);

  /* Fall out if recursive interrupt */
  if(SBIRQBusy) { _enable();return; } else { SBIRQBusy=1;_enable(); }

  // @@@ ADD OPTIONAL SOUND SYNTHESIS CODE HERE!

  /* Done */
  SBIRQBusy=0;
}

/** ReadDSP() ************************************************/
/** Read a value from SoundBlaster. Returns -1 if failed.   **/
/*************************************************************/
static int ReadDSP(void)
{
  register int J;

  for(J=0;J<100000;J++)
    if(inp(SBPort+0x0E)&0x80)
      return(inp(SBPort+0x0A));
  return(-1);
}

/** WriteDSP() ***********************************************/
/** Write a value into SoundBlaster. Returns 0 if failed,   **/
/** 1 otherwise.                                            **/
/*************************************************************/
static int WriteDSP(register int V)
{
  register int J;

  for(J=0;J<100000;J++)
    if(!(inp(SBPort+0x0C)&0x80))
    { outp(SBPort+0x0C,V);return(1); }
  return(0);
}

/** WriteMixer() *********************************************/
/** Write a value into SoundBlaster's mixer.                **/
/*************************************************************/
static void WriteMixer(register int R,register int V)
{
  outp(SBPort+0x04,R);
  outp(SBPort+0x05,V);
}

/** GetSegment() *********************************************/
/** Get a segment of given size in the DOS low memory (i.e. **/
/** <1MB). Returns segment descriptor on success, or 0 on   **/
/** failure.                                                **/
/*************************************************************/
unsigned short GetSegment(unsigned short Paragraphs)
{
  union REGS R;

  R.w.ax = 0x0100;
  R.w.bx = Paragraphs;
  int386(0x31,&R,&R);
  return(R.w.cflag? 0:R.w.ax);
}

/** FreeSegment() ********************************************/
/** Free a segment allocated with GetSegment().             **/
/*************************************************************/
void FreeSegment(unsigned short Segment)
{
  union REGS R;

  R.w.ax = 0x0101;
  R.w.dx = Segment;
  int386(0x31,&R,&R);
}

/*************************************************************/
/** The following part should be compiled with GIFLIB.      **/
/*************************************************************/
#ifndef GIFLIB
int LoadGIF(const char *File) { return(0); }
int SaveGIF(const char *File) { return(0); }
#else /* GIFLIB */

/** LoadGIF() ************************************************/
/** Read picture from a .GIF file to screen. Returns the    **/
/** number of allocated colors or 0 if failed.              **/
/*************************************************************/
int LoadGIF(const char *File)
{
  static const int InterlacedOffset[] = { 0, 4, 2, 1 };
  static const int InterlacedJumps[]  = { 8, 8, 4, 2 };

  GifFileType *D;
  GifRecordType RecordType;
  GifByteType *Extension;
  GifPixelType *Buf;
  GifColorType *Colors;
  int I,J,K,H,W,Lines,ExtCode,Width,Height;
  char *P;

  /* If not in the graphics mode, fall out */
  if(!VGAMode) return(0);
  /* If can't open .GIF file, fall out */
  if(!(D=DGifOpenFileName(File))) return(0);

  for(K=1,Lines=0;K&&DGifGetRecordType(D,&RecordType);)
    switch(RecordType)
    {
      case TERMINATE_RECORD_TYPE:
        K=0;break;
      case EXTENSION_RECORD_TYPE:
        if(DGifGetExtension(D,&ExtCode,&Extension))
          while(Extension)
            if(!DGifGetExtensionNext(D,&Extension)) break;
        break;
      case IMAGE_DESC_RECORD_TYPE:
        /* Get image description */
        if(!DGifGetImageDesc(D)) break;
        /* .GIF file must not be interlaced */
        if(D->IInterlace) break;
        /* Get image width/height */
        Width=D->IWidth? D->IWidth:D->SWidth;
        Height=D->IHeight? D->IHeight:D->SHeight;
        /* Get screen start/width/height */
        if(VGAMode==2) { P=(char *)0xA0000;W=256,H=240; }
        else           { P=(char *)0xA0000;W=320,H=200; }
        /* Allocate buffer and read the lines */
        if(Buf=(GifPixelType *)malloc(Width))
        {
          /* Skip extra lines */
          if(H>Height) P+=W*(H-Height)/2;
          else
            for(I=(Height-H)/2;I>=0;I--)
              if(!DGifGetLine(D,Buf,Width)) break;
          /* Read needed number of lines */
          for(Lines=0;Lines<H;Lines++,P+=W)
            if(!DGifGetLine(D,Buf,Width)) break;
            else
            {
              if(W>Width) { I=(W-Width)/2;J=0;K=Width; }
              else        { J=(Width-W)/2;I=0;K=W; }
              for(;K;K--) P[I++]=255-Buf[J++];
            }
          K=1;free(Buf);
        }
        break;
    }

  /* Set VGA palette */
  if(!Lines) I=0;
  else
  {
    Colors=D->IColorMap? D->IColorMap:D->SColorMap;
    I=1<<(D->IColorMap? D->IBitsPerPixel:D->SBitsPerPixel);
    outp(0x3C8,0);
    for(K=0;K<I;K++)
    {
      outp(0x3C8,255-K);
      outp(0x3C9,Colors[K].Red>>2);
      outp(0x3C9,Colors[K].Green>>2);
      outp(0x3C9,Colors[K].Blue>>2);
    }
  }

  DGifCloseFile(D);
  return(I);
}

/** SaveGIF() ************************************************/
/** Save screen into a .GIF file. Returns the number of     **/
/** scanlines written or 0 if failed.                       **/
/*************************************************************/
int SaveGIF(const char *File)
{
  GifFileType *D;
  GifColorType Colors[256];
  int J,W,H;
  char *P;

  /* If not in the graphics mode, fall out */
  if(!VGAMode) return(0);
  /* If can't open .GIF file, fall out */
  if(!(D=EGifOpenFileName(File,0))) return(0);

  /* Read the palette */
  outp(0x3C7,0);
  for(J=0;J<256;J++)
  {
    Colors[J].Red=(inp(0x3C9)&0x3F)<<2;
    Colors[J].Green=(inp(0x3C9)&0x3F)<<2;
    Colors[J].Blue=(inp(0x3C9)&0x3F)<<2;
  }

  J=0;
  if(VGAMode==2) { P=(char *)0xA0000;W=256;H=240; }
  else           { P=(char *)0xA0000;W=320;H=200; }
  if(EGifPutScreenDesc(D,W,H,8,0,8,&Colors[0]))
    if(EGifPutImageDesc(D,0,0,W,H,0,8,&Colors[0]))
        for(J=0;J<H;J++,P+=W)
          if(!EGifPutLine(D,P,W)) break;

  EGifCloseFile(D);
  return(J);
}

#endif /* GIFLIB */
