/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                        Coleco.c                         **/
/**                                                         **/
/** This file contains implementation for the Coleco        **/
/** specific hardware. Initialization code is also here.    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "Coleco.h"
#include "Sound.h"

#ifdef WII
#include "wii_app.h"
#include "wii_coleco.h"
#endif

#ifdef WII_NETTRACE
#include <network.h>
#include "net_print.h"  
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#ifdef __WATCOMC__
#include <direct.h>
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

#ifdef WII
int  CartSize    = 0;          /* The loaded cartridge's size   */
#endif
byte Verbose     = 1;          /* Debug msgs ON/OFF             */
byte UPeriod     = 75;         /* Percentage of frames to draw  */
int  Mode        = 0;          /* Conjunction of CV_* bits      */

int  ScrWidth    = 272;        /* Screen buffer width           */
int  ScrHeight   = 208;        /* Screen buffer height          */
void *ScrBuffer  = 0;          /* If screen buffer allocated,   */
                               /* put address here              */

Z80 CPU;                       /* Z80 CPU state                 */
SN76489 PSG;                   /* SN76489 PSG state             */
TMS9918 VDP;                   /* TMS9918 VDP state             */

byte *RAM;                     /* CPU address space             */
byte *ROMPage[8];              /* 8x8kB read-only (ROM) pages   */
byte *RAMPage[8];              /* 8x8kB read-write (RAM) pages  */
byte Port20;                   /* Adam port 0x20-0x3F (AdamNet) */
byte Port60;                   /* Adam port 0x60-0x7F (memory)  */

byte ExitNow;                  /* 1: Exit the emulator          */
byte AdamROMs;                 /* 1: All Adam ROMs are loaded   */ 

byte JoyMode;                  /* Joystick controller mode      */
unsigned int JoyState;         /* Joystick states               */
unsigned int SpinCount;        /* Spinner counters              */
unsigned int SpinStep;         /* Spinner steps                 */
unsigned int SpinState;        /* Spinner bit states            */

char *SndName    = "LOG.MID";  /* Soundtrack log file           */
char *StaName    = 0;          /* Emulation state save file     */
char *HomeDir    = 0;          /* Full path to home directory   */
char *PrnName    = 0;          /* Printer redirection file      */
FILE *PrnStream;

#ifdef MEGACART
#define MAX_MEGACART_SIZE 0x100000
unsigned int MegaCartMask = 0;
#endif

#ifdef ZLIB
#define fopen           gzopen
#define fclose          gzclose
#define fread(B,N,L,F)  gzread(F,B,(L)*(N))
#define fwrite(B,N,L,F) gzwrite(F,B,(L)*(N))
#endif

#ifdef WII
// The Coleco ROM
static char coleco_rom[WII_MAX_PATH] = "";
#endif

/** StartColeco() ********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartColeco(const char *Cartridge)
{
  char CurDir[256];
  FILE *F;
  int *T,J;
  char *P;

  /*** STARTUP CODE starts here: ***/
  T=(int *)"\01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#ifdef LSB_FIRST
  if(*T!=1)
  {
    printf("********** This machine is high-endian. **********\n");
    printf("Take #define LSB_FIRST out and compile ColEm again.\n");
    return(0);
  }
#else
  if(*T==1)
  {
    printf("********* This machine is low-endian. **********\n");
    printf("Insert #define LSB_FIRST and compile ColEm again.\n");
    return(0);
  }
#endif

  /* Zero everything */
  RAM      = 0;
  ExitNow  = 0;
  AdamROMs = 0;
  StaName  = 0;

  /* Set up CPU modes */
  CPU.TrapBadOps = Verbose&0x04;
  CPU.IAutoReset = 1;

  /* Allocate memory for RAM and ROM */
#ifdef MEGACART
  unsigned int addressSpace = 0x40000-0x8000+MAX_MEGACART_SIZE;
  if(!(RAM=malloc(addressSpace))) { if(Verbose) puts("FAILED");return(0); }
  memset(RAM,NORAM,addressSpace);
#else  
  if(Verbose) printf("Allocating 256kB for CPU address space...");  
  if(!(RAM=malloc(0x40000))) { if(Verbose) puts("FAILED");return(0); }
  memset(RAM,NORAM,0x40000);
#endif

  /* Create TMS9918 VDP */
  if(Verbose) printf("OK\nCreating VDP...");
  ScrBuffer=New9918(&VDP,ScrBuffer,ScrWidth,ScrHeight);
  if(!ScrBuffer) { if(Verbose) puts("FAILED");return(0); }
  VDP.DrawFrames=UPeriod;

  /* Loading the ROMs... */
  if(Verbose) printf("OK\nLoading ROMs:\n");
  P=0;

  /* Go to home directory if specified */
  if(!HomeDir) CurDir[0]='\0';
  else
  {
    if(!getcwd(CurDir,sizeof(CurDir))) CurDir[0]='\0';
    chdir(HomeDir);
  }

  /* COLECO.ROM: OS7 (ColecoVision BIOS) */
  if(Verbose) printf("  Opening COLECO.ROM...");
#ifdef WII
  // TODO: Fix this
  char rom[WII_MAX_PATH] = "";

  if( coleco_rom[0] == '\0' )
  {
    snprintf( coleco_rom, WII_MAX_PATH, "%s%s", 
      wii_get_fs_prefix(), WII_COLECO_ROM );
  }
  if(!(F=fopen(coleco_rom,"rb"))) { P="NOT FOUND"; }
#else
  if(!(F=fopen("COLECO.ROM","rb"))) { P="NOT FOUND"; }
#endif
  else
  {
    if(fread(ROM_BIOS,1,0x2000,F)!=0x2000) P="SHORT FILE";
    fclose(F);
  }

  /* WRITER.ROM: SmartWriter (Adam bootup) */
  if(!P)
  {
    if(Verbose) printf("OK\n  Opening WRITER.ROM...");

#ifdef WII
    // TODO: Fix this
    wii_get_app_relative( "WRITER.ROM", rom );
    if((F=fopen(rom,"rb")))
#else
    if(F=fopen("WRITER.ROM","rb"))
#endif
    {
      if(fread(ROM_WRITER,1,0x8000,F)==0x8000) ++AdamROMs;
      fclose(F);
    }
    if(Verbose) printf(AdamROMs>=1? "OK\n":"FAILED\n");
  }

  /* EOS.ROM: EOS (Adam BIOS) */
  if(!P&&AdamROMs)
  {
    if(Verbose) printf("  Opening EOS.ROM...");

#ifdef WII
    // TODO: Fix this
    wii_get_app_relative( "WRITER.ROM", rom );
    if((F=fopen(rom,"rb")))
#else
    if(F=fopen("EOS.ROM","rb"))
#endif    
    {
      if(fread(ROM_EOS,1,0x2000,F)==0x2000) ++AdamROMs;
      fclose(F);
    }
    if(Verbose) printf(AdamROMs>=2? "OK\n":"FAILED\n");
  }

  /* If not all Adam ROMs loaded, disable Adam */
  AdamROMs=AdamROMs>=2;
  if(!AdamROMs) Mode&=~CV_ADAM;

  /* Done loading ROMs */
  if(CurDir[0]) chdir(CurDir);
  if(P) { if(Verbose) printf("%s\n",P);return(0); }

  /* Open stream for a printer */
  if(!PrnName) PrnStream=stdout;
  else
  {
    if(Verbose) printf("Redirecting printer output to %s...",PrnName);
    if(!(PrnStream=fopen(PrnName,"wb"))) PrnStream=stdout;
    if(Verbose) printf(PrnStream==stdout? "FAILED\n":"OK\n");
  }

  /* Initialize MIDI sound logging */
  InitMIDI(SndName);

  /* Reset system hardware */
  ResetColeco(Mode);

  /* Load cartridge */
  if(Cartridge)
  {
    if(Verbose) printf("  Opening %s...",Cartridge);
    J=LoadROM(Cartridge);
    if(Verbose)
    {
      if(J) printf("%d bytes loaded...OK\n",J);
      else  printf("FAILED\n");
    }
  }

  if(Verbose) printf("RUNNING ROM CODE...\n");
  J=RunZ80(&CPU);

  if(Verbose) printf("EXITED at PC = %04Xh.\n",J);
  return(1);
}

/** LoadROM() ************************************************/
/** Load given cartridge ROM file. Returns number of bytes  **/
/** on success, 0 on failure.                               **/
/*************************************************************/
int LoadROM(const char *Cartridge)
{
  byte Buf[2],*P;
  FILE *F;
  int J;

#ifdef WII
  // Fixes bug that occurred when you load a save state for a
  // cartridge w/ a size smaller than the previous cart 
  // (CRC fails)
#ifdef MEGACART
  memset(ROM_CARTRIDGE,0x0,MAX_MEGACART_SIZE);
#else
  memset(ROM_CARTRIDGE,0x0,0x8000);
#endif
#endif

  /* Open file */
  F=fopen(Cartridge,"rb");
  if(!F) return(0);

  /* Read magic number */
  if(fread(Buf,1,2,F)!=2) { fclose(F);return(0); }

  /* If it is a ColecoVision game cartridge... */
  /* If it is a Coleco Adam expansion ROM... */
  P = (Buf[0]==0x55)&&(Buf[1]==0xAA)? ROM_CARTRIDGE
    : (Buf[0]==0xAA)&&(Buf[1]==0x55)? ROM_CARTRIDGE
    : (Buf[0]==0x66)&&(Buf[1]==0x99)? ROM_EXPANSION
    : 0;

#ifdef WII_NETTRACE
  char val[256];
  sprintf( val, "Cartidge type: %p\n", P );
  net_print_string(__FILE__,__LINE__, val );  
#endif
  
#ifdef MEGACART
  if(!P) P = ROM_CARTRIDGE;
#else
  /* If ROM not recognized, drop out */
  if(!P) { fclose(F);return(0); }
#endif

  /* Read the rest of the ROM */
  P[0]=Buf[0];
  P[1]=Buf[1];
#ifdef MEGACART
  J=2+fread(P+2,1,MAX_MEGACART_SIZE-2,F); 
#else
  J=2+fread(P+2,1,0x7FFE,F);
#endif

#ifdef WII
  // Retain the current cartridge's size
  CartSize = J;
#endif

#ifdef WII_NETTRACE  
  sprintf( val, "Cartidge size: %dk\n", CartSize / 1024 );
  net_print_string(__FILE__,__LINE__, val );  
#endif

#ifdef MEGACART
  MegaCartMask = ( CartSize > 0x8000 ? ( CartSize / 0x4000 - 1 ) : 0 );

#ifdef WII_NETTRACE  
  sprintf( val, "Cartridge: %s\n", ( MegaCartMask ? "MegaCart" : "Normal" ) );
  net_print_string(__FILE__,__LINE__, val );  
#endif
#endif

  /* Done with the file */
  fclose(F);

  /* Reset hardware */
  ResetColeco(Mode);

#ifndef WII
  /* Free previous state name */
  if(StaName) free(StaName);

  /* If allocated enough space for the state name... */
  if(StaName=malloc(strlen(Cartridge)+4))
  {
    /* Compose state file name */
    strcpy(StaName,Cartridge);
    P=strrchr(StaName,'.');
    if(P) strcpy(P,".sta"); else strcat(StaName,".sta");
    /* Try loading state file */
    LoadSTA(StaName);
  }
#endif

  /* Done */
  return(J);
}

/** SaveSTA() ************************************************/
/** Save emulation state to a given file. Returns 1 on      **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int SaveSTA(const char *StateFile)
{
  static byte Header[16] = "STF\032\001\0\0\0\0\0\0\0\0\0\0\0";
  unsigned int State[256],J;
  FILE *F;

  /* Open saved state file */
  if(!(F=fopen(StateFile,"wb"))) return(0);

  /* Fill header */
  J=CartCRC();
  Header[5] = Mode&CV_ADAM;
  Header[6] = J&0xFF;
  Header[7] = (J>>8)&0xFF;
  Header[8] = (J>>16)&0xFF;
  Header[9] = (J>>24)&0xFF;
  
  /* Write out the header */
  if(fwrite(Header,1,16,F)!=16) { fclose(F);return(0); }

  /* Generate hardware state */
  J=0;
  memset(State,0,sizeof(State));
  State[J++] = Mode;
  State[J++] = UPeriod;
  State[J++] = ROMPage[0]-RAM;
  State[J++] = ROMPage[1]-RAM;
  State[J++] = ROMPage[2]-RAM;
  State[J++] = ROMPage[3]-RAM;
  State[J++] = ROMPage[4]-RAM;
  State[J++] = ROMPage[5]-RAM;
  State[J++] = ROMPage[6]-RAM;
  State[J++] = ROMPage[7]-RAM;
  State[J++] = RAMPage[0]-RAM;
  State[J++] = RAMPage[1]-RAM;
  State[J++] = RAMPage[2]-RAM;
  State[J++] = RAMPage[3]-RAM;
  State[J++] = RAMPage[4]-RAM;
  State[J++] = RAMPage[5]-RAM;
  State[J++] = RAMPage[6]-RAM;
  State[J++] = RAMPage[7]-RAM;
  State[J++] = JoyMode;
  State[J++] = Port20;
  State[J++] = Port60;

  /* Write out CPU state */
  if(fwrite(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);return(0); }

  /* Write out VDP state */
  if(fwrite(&VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);return(0); }
  
  /* Write out PSG state */
  if(fwrite(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);return(0); }

  /* Write out hardware state */
  if(fwrite(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);return(0); }

  /* Write out memory contents */
  if(fwrite(RAM_BASE,1,0xA000,F)!=0xA000)
  { fclose(F);return(0); }
  if(fwrite(VDP.VRAM,1,0x4000,F)!=0x4000)
  { fclose(F);return(0); }

#ifdef WII
  if( wii_coleco_db_entry.flags&LOTD_SRAM )
  {
      // Write out the Lord of the Dungeon SRAM
      if(fwrite(ROM_CARTRIDGE+0x6000,1,0x2000,F)!=0x2000)
      { fclose(F);return(0); }    
  }
#endif

#ifdef WII_NETTRACE
  int i;
  char temp[512];
  sprintf( temp, "RAM_MAIN_LO=0x%x\n", RAM_MAIN_LO-RAM );
  net_print_string( "", 0, temp);  
  sprintf( temp, "RAM_MAIN_HI=0x%x\n", RAM_MAIN_HI-RAM );
  net_print_string( "", 0, temp);  
  for( i = 0; i < 7; i++ )
  {
    sprintf( temp, "ROMPage[%d]=0x%x-0x%x\n", i, 
      ROMPage[i]-RAM, ROMPage[i]+0x2000-RAM );
    net_print_string( "", 0, temp);  
  }
  for( i = 0; i < 7; i++ )
  {
    sprintf( temp, "RAMPage[%d]=0x%x-0x%x\n", i, 
      RAMPage[i]-RAM, RAMPage[i]+0x2000-RAM );
    net_print_string( "", 0, temp);  
  }
#endif

  /* Done */
  fclose(F);
  return(1);
}

/** LoadSTA() ************************************************/
/** Load emulation state from a given file. Returns 1 on    **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int LoadSTA(const char *StateFile)
{
  unsigned int State[256],J;
  byte Header[16],*VRAM;
#ifndef WII
  int XPal[16];
#endif
  void *XBuf;
  FILE *F;

  /* Open saved state file */
  if(!(F=fopen(StateFile,"rb"))) return(0);

  /* Read and check the header */
  if(fread(Header,1,16,F)!=16)
  { fclose(F);return(0); }

  if(memcmp(Header,"STF\032\001",5))
  { fclose(F);return(0); }
  J=CartCRC();

  if(
    (Header[5]!=(Mode&CV_ADAM)) ||
    (Header[6]!=(J&0xFF))       ||
    (Header[7]!=((J>>8)&0xFF))  ||
    (Header[8]!=((J>>16)&0xFF)) ||
    (Header[9]!=((J>>24)&0xFF))
  ) { fclose(F);return(0); }

  /* Read CPU state */
  if(fread(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);ResetColeco(Mode);return(0); }

  /* Read VDP state, preserving VRAM address */
  VRAM        = VDP.VRAM;
  XBuf        = VDP.XBuf;
#ifndef WII
  memcpy(XPal,VDP.XPal,sizeof(XPal));
#endif
  if(fread(&VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);VDP.VRAM=VRAM;VDP.XBuf=XBuf;ResetColeco(Mode);return(0); }
  VDP.ChrTab += VRAM-VDP.VRAM;
  VDP.ChrGen += VRAM-VDP.VRAM;
  VDP.SprTab += VRAM-VDP.VRAM;
  VDP.SprGen += VRAM-VDP.VRAM;
  VDP.ColTab += VRAM-VDP.VRAM;
  VDP.VRAM    = VRAM;
  VDP.XBuf    = XBuf;
#ifndef WII
  memcpy(VDP.XPal,XPal,sizeof(VDP.XPal));
#endif

  /* Read PSG state */
  if(fread(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);ResetColeco(Mode);return(0); }

  /* Read hardware state */
  if(fread(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);ResetColeco(Mode);return(0); }

  /* Read memory contents */
  if(fread(RAM_BASE,1,0xA000,F)!=0xA000)
  { fclose(F);ResetColeco(Mode);return(0); }
  if(fread(VDP.VRAM,1,0x4000,F)!=0x4000)
  { fclose(F);ResetColeco(Mode);return(0); }

#ifdef WII
  if( wii_coleco_db_entry.flags&LOTD_SRAM )
  {
      // Read back the Lord of the Dungeon SRAM
      unsigned int readBytes = fread(ROM_CARTRIDGE+0x6000,1,0x2000,F);
      if(readBytes!=0x2000&&readBytes!=0x0) // 0x0 is for back compatibility
      { fclose(F);return(0); }    
  }
#endif

  /* Done with the file */
  fclose(F);

  /* Parse hardware state */
  J=0;
  Mode       = State[J++];
  UPeriod    = State[J++];
  ROMPage[0] = State[J++]+RAM;
  ROMPage[1] = State[J++]+RAM;
  ROMPage[2] = State[J++]+RAM;
  ROMPage[3] = State[J++]+RAM;
  ROMPage[4] = State[J++]+RAM;
  ROMPage[5] = State[J++]+RAM;
  ROMPage[6] = State[J++]+RAM;
  ROMPage[7] = State[J++]+RAM;
  RAMPage[0] = State[J++]+RAM;
  RAMPage[1] = State[J++]+RAM;
  RAMPage[2] = State[J++]+RAM;
  RAMPage[3] = State[J++]+RAM;
  RAMPage[4] = State[J++]+RAM;
  RAMPage[5] = State[J++]+RAM;
  RAMPage[6] = State[J++]+RAM;
  RAMPage[7] = State[J++]+RAM;
  JoyMode    = State[J++];
  Port20     = State[J++];
  Port60     = State[J++];

  /* All PSG channels have been changed */
  PSG.Changed = 0xFF;

  /* Done */
  return(1);
}

#ifdef ZLIB
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#endif

/** TrashColeco() ********************************************/
/** Free memory allocated by StartColeco().                 **/
/*************************************************************/
void TrashColeco(void)
{
  /* Free all memory and resources */
  if(RAM)     free(RAM);
  if(StaName) free(StaName);

  /* Close MIDI sound log */
  TrashMIDI();

  /* Done with VDP */
  Trash9918(&VDP);
}

/** SetMemory() **********************************************/
/** Set memory pages according to passed values.            **/
/*************************************************************/
void SetMemory(byte NewPort60,byte NewPort20)
{
  /* Store new values */
  Port60 = NewPort60;
  Port20 = NewPort20;

  /* Lower 32kB ROM */
  if(!(NewPort60&0x03)&&(NewPort20&0x02))
  {
    ROMPage[0] = RAM_DUMMY;
    ROMPage[1] = RAM_DUMMY;
    ROMPage[2] = ROM_EOS;
    ROMPage[3] = ROM_EOS;
  }
  else
  {
    ROMPage[0] = RAM+((int)(NewPort60&0x03)<<15);
    if(!(Mode&CV_ADAM))
    {
      ROMPage[1] = RAM_DUMMY;
      ROMPage[2] = RAM_DUMMY;
      ROMPage[3] = RAM_BASE;
    }
    else
    {
      ROMPage[1] = ((NewPort60&0x03)==3? RAM_MAIN_LO:ROMPage[0])+0x2000;
      ROMPage[2] = ROMPage[1]+0x2000;
      ROMPage[3] = ROMPage[1]+0x4000;
    }
  }

  /* Upper 32kB ROM */
#ifdef MEGACART
  if( MegaCartMask )
  {
    ROMPage[4] = ROM_CARTRIDGE+(MegaCartMask << 14);
    ROMPage[5] = ROMPage[4]+0x2000;
    ROMPage[6] = ROM_CARTRIDGE;
    ROMPage[7] = ROMPage[6]+0x2000;
  }
  else
  {
#endif
    ROMPage[4] = RAM+((int)(NewPort60&0x0C)<<13)+0x20000;
    ROMPage[5] = ROMPage[4]+0x2000;
    ROMPage[6] = ROMPage[4]+0x4000;
    ROMPage[7] = ROMPage[4]+0x6000;
#ifdef MEGACART
  }
#endif

  /* Lower 32kB RAM */
  if(!(Port60&0x03))
  {
    RAMPage[0]=RAMPage[1]=RAMPage[2]=RAMPage[3]=RAM_DUMMY;
  }
  else
  {
    RAMPage[0] = (Port60&0x03)==3? RAM_DUMMY:ROMPage[0]; 
    RAMPage[1] = ROMPage[1];
    RAMPage[2] = ROMPage[2];
    RAMPage[3] = ROMPage[3];

#ifdef WII
    if( wii_coleco_db_entry.flags&OPCODE_MEMORY )
    {
#ifdef WII_NETTRACE
      net_print_string(__FILE__,__LINE__, "Opcode memory enabled.\n" );  
#endif
      ROMPage[1] = RAMPage[1] = RAM_MAIN_HI;
      ROMPage[2] = RAMPage[2] = RAMPage[1]+0x2000;
    }
#endif
  }

  /* Upper 32kB RAM */
  if(Port60&0x04)
    RAMPage[4]=RAMPage[5]=RAMPage[6]=RAMPage[7]=RAM_DUMMY;
  else
  {
    RAMPage[4] = ROMPage[4];
    RAMPage[5] = ROMPage[5];
    RAMPage[6] = ROMPage[6];
    RAMPage[7] = ROMPage[7];
  }
}

/** CartCRC() ************************************************/
/** Compute cartridge CRC.                                  **/
/*************************************************************/
unsigned int CartCRC(void)
{
  unsigned int I,J;
#ifdef WII
  // The CRC was taking into consideration all of the cartridge RAM when 
  // calculating the CRC. In reality, it should only calculate based on
  // the size of the cartridge. This simple fix ignores anything after
  // the cartridge's size. This is done to retain backward compatibility
  // with previous saves.

#ifdef MEGACART
  unsigned int maxCartSize = ( MegaCartMask ? MAX_MEGACART_SIZE : 0x8000 );
#else
  unsigned int maxCartSize = 0x8000;
#endif 

  unsigned int size = CartSize;
  if( wii_coleco_db_entry.flags&LOTD_SRAM && size == 0x8000 )
  {
    // 32k version is really 24k plus SRAM
    size = 0x6000;
  }

  for(J=I=0;J<maxCartSize;++J) 
  {
    if( J < size )
    {
      I+=ROM_CARTRIDGE[J];
    }
    else
    { 
      I+=0xFF; // Old NORAM value
    }      
  }
#else
  for(J=I=0;J<0x8000;++J) I+=ROM_CARTRIDGE[J];
#endif
  return(I);
}

/** ResetColeco() ********************************************/
/** Reset CPU and hardware to new operating modes. Returns  **/
/** new value of the Mode variable (possibly != NewMode).   **/
/*************************************************************/
int ResetColeco(int NewMode)
{
  int I,J;

  /* Disable Adam if not all ROMs are loaded */
  if(!AdamROMs) NewMode&=~CV_ADAM;

  /* Set new modes into effect */
  Mode      = NewMode;

  /* Initialize hardware */
  JoyMode   = 0;
  JoyState  = 0;
  SpinState = 0;
  SpinCount = 0;
  SpinStep  = 0;

  /* Clear memory (important for NetPlay, to  */
  /* keep states at both sides consistent)    */
  memset(RAM_MAIN_LO,NORAM,0x8000);
  memset(RAM_MAIN_HI,NORAM,0x8000);
  memset(RAM_EXP_LO,NORAM,0x8000);
  memset(RAM_EXP_HI,NORAM,0x8000);
  memset(RAM_OS7,NORAM,0x2000);

  /* Set up memory pages */
  SetMemory(Mode&CV_ADAM? 0x00:0x0F,0x00);

  /* Reset TMS9918 VDP */
  Reset9918(&VDP,ScrBuffer,ScrWidth,ScrHeight);
  /* Set sprite limit */
  VDP.MaxSprites = Mode&CV_ALLSPRITE? 255:TMS9918_MAXSPRITES; 
  /* Reset SN76489 PSG */
  Reset76489(&PSG,0);
  Sync76489(&PSG,SN76489_SYNC);
  /* Reset Z80 CPU */
  ResetZ80(&CPU);

  /* Set scanline parameters according to video type */
  VDP.Scanlines = Mode&CV_PAL? TMS9929_LINES:TMS9918_LINES;
  CPU.IPeriod   = Mode&CV_PAL? TMS9929_LINE:TMS9918_LINE;

  /* Set up the palette */
  I = Mode&CV_PALETTE;
  I = I==CV_PALETTE0? 0:I==CV_PALETTE1? 16:I==CV_PALETTE2? 32:0;
  for(J=0;J<16;++J,++I)
    VDP.XPal[J]=SetColor(J,Palette9918[I].R,Palette9918[I].G,Palette9918[I].B);

  /* Return new modes */
  return(Mode);
}

/** WrZ80() **************************************************/
/** Z80 emulation calls this function to write byte V to    **/
/** address A of Z80 address space.                         **/
/*************************************************************/
void WrZ80(register word A,register byte V)
{
#ifndef WII
  if(Mode&CV_ADAM) RAMPage[A>>13][A&0x1FFF]=V;
  else if((A>=0x6000)&&(A<0x8000))
       {
         A&=0x03FF;
         RAM_BASE[A]       =RAM_BASE[0x0400+A]=
         RAM_BASE[0x0800+A]=RAM_BASE[0x0C00+A]=
         RAM_BASE[0x1000+A]=RAM_BASE[0x1400+A]=
         RAM_BASE[0x1800+A]=RAM_BASE[0x1C00+A]=V;
       }
#else
  if(Mode&CV_ADAM) 
  {
    RAMPage[A>>13][A&0x1FFF]=V;
  }
  else
  {
    if((A>=0x6000)&&(A<0x8000))
    {
      A&=0x03FF;
      RAM_BASE[A]       =RAM_BASE[0x0400+A]=
      RAM_BASE[0x0800+A]=RAM_BASE[0x0C00+A]=
      RAM_BASE[0x1000+A]=RAM_BASE[0x1400+A]=
      RAM_BASE[0x1800+A]=RAM_BASE[0x1C00+A]=V;
    }
    else if((wii_coleco_db_entry.flags&LOTD_SRAM)&&(A>=0xE000)&&(A<0xE800))
    {
      // To support Lord of the Dungeon writes to SRAM
      A&=0x07FF;
      ROM_CARTRIDGE[0x6000+A]=ROM_CARTRIDGE[0x6800+A]=
      ROM_CARTRIDGE[0x7000+A]=ROM_CARTRIDGE[0x7800+A]=V;
    }
    else if((wii_coleco_db_entry.flags&OPCODE_MEMORY)&&(A>=0x2000)&&(A<0x5FFF))
	  {
      // To support Opcode RAM expansion
	    RAMPage[A>>13][A&0x1FFF]=V;
	  }
  }
#endif
}

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A of Z80 address space. Copied to Z80.c and     **/
/** made inlined to speed things up.                        **/
/*************************************************************/
byte RdZ80(register word A) { return(ROMPage[A>>13][A&0x1FFF]); }

/** PatchZ80() ***********************************************/
/** Z80 emulation calls this function when it encounters a  **/
/** special patch command (ED FE) provided for user needs.  **/
/*************************************************************/
void PatchZ80(Z80 *R) {}

/** InZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** a given I/O port.                                       **/
/*************************************************************/
byte InZ80(register word Port)
{
  /* Coleco uses 8bit port addressing */
  Port&=0x00FF;

  switch(Port&0xE0)
  {
    case 0x40: /* Printer Status */
      if((Mode&CV_ADAM)&&(Port==0x40)) return(0xFF);
      break;

    case 0x20: /* AdamNet Control */
      if(Mode&CV_ADAM) return(Port20);
      break;

    case 0x60: /* Memory Control */
      if(Mode&CV_ADAM) return(Port60);
      break;

    case 0xE0: /* Joysticks Data */
      Port = Port&0x02? (JoyState>>16):JoyState;
      Port = JoyMode?   (Port>>8):Port;
      return(~Port&0x7F);

    case 0xA0: /* VDP Status/Data */
      return(Port&0x01? RdCtrl9918(&VDP):RdData9918(&VDP));
  }

  /* No such port */
#ifdef WII
  return(0xFF); // Old NORAM value
#else
  return(NORAM);
#endif
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write a byte to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(register word Port,register byte Value)
{
  /* Coleco uses 8bit port addressing */
  Port&=0x00FF;

  switch(Port&0xE0)
  {
    case 0x80: JoyMode=0;break;
    case 0xC0: JoyMode=1;break;
    case 0xE0: Write76489(&PSG,Value);break; 

    case 0xA0:
      if(!(Port&0x01)) WrData9918(&VDP,Value);
      else if(WrCtrl9918(&VDP,Value)) CPU.IRequest=INT_NMI;
      break;

    case 0x40:
      if((Mode&CV_ADAM)&&(Port==0x40)) fputc(Value,PrnStream);
      break;

    case 0x20:
      if(Mode&CV_ADAM) SetMemory(Port60,Value);
      break;

    case 0x60:
      if(Mode&CV_ADAM) SetMemory(Value,Port20);
      break;
  }
}

/** LoopZ80() ************************************************/
/** Z80 emulation calls this function periodically to check **/
/** if the system hardware requires any interrupts.         **/
/*************************************************************/
word LoopZ80(Z80 *R)
{
  static byte ACount=0;

  /* If emulating spinners... */
  if(Mode&CV_SPINNERS)
  {
    /* Reset spinner bits */
    JoyState&=~0x30003000;

    /* Count ticks for both spinners */
    SpinCount+=SpinStep;

    /* Process first spinner */
    if(SpinCount&0x00008000)
    {
      SpinCount&=~0x00008000;
      if(Mode&CV_SPINNER1)
      {
        JoyState   |= SpinState&0x00003000;
        R->IRequest = INT_RST38;
      }
    }

    /* Process second spinner */
    if(SpinCount&0x80000000)
    {
      SpinCount&=~0x80000000;
      if(Mode&CV_SPINNER2)
      {
        JoyState   |= SpinState&0x30000000;
        R->IRequest = INT_RST38;
      }
    }
  }

  /* Refresh VDP */
  if(Loop9918(&VDP)) R->IRequest=INT_NMI;
  
  /* Drop out unless end of screen is reached */
  if(VDP.Line!=TMS9918_END_LINE) return(R->IRequest);

  /* End of screen reached... */

  /* Check joysticks, clear unused bits */
  JoyState=Joystick()&~0x30003000;

  /* Lock out opposite direction keys (Grog's Revenge) */
  if(JoyState&JST_RIGHT)       JoyState&=~JST_LEFT;
  if(JoyState&JST_DOWN)        JoyState&=~JST_UP;
  if(JoyState&(JST_RIGHT<<16)) JoyState&=~(JST_LEFT<<16);
  if(JoyState&(JST_DOWN<<16))  JoyState&=~(JST_UP<<16);

  /* If emulating spinners... */
  if(Mode&CV_SPINNERS)
  {
    int I,K;

    /* Get mouse position relative to the window center, */
    /* normalized to -512..+512 range */
    I = Mouse();
    /* First spinner */
    K = (Mode&CV_SPINNER1Y? (I<<2):Mode&CV_SPINNER1X? (I<<16):0)>>16;
    K = K<-512? -512:K>512? 512:K;
    SpinStep  = K>=0? (K>32? K:0):(K<-32? -K:0);    
    SpinState = K>0? 0x00003000:K<0? 0x00001000:0;
    /* Second spinner */
    K = (Mode&CV_SPINNER2Y? (I<<2):Mode&CV_SPINNER2X? (I<<16):0)>>16;
    K = K<-512? -512:K>512? 512:K;
    SpinStep |= (K>=0? (K>32? K:0):(K<-32? -K:0))<<16;    
    SpinState|= K>0? 0x10000000:K<0? 0x30000000:0;
    /* Fire buttons */
    if(I&0x80000000)
      JoyState |= (Mode&CV_SPINNER2? (JST_FIRER<<16):0)
               |  (Mode&CV_SPINNER1? JST_FIRER:0);
    if(I&0x40000000)
      JoyState |= (Mode&CV_SPINNER2? (JST_FIREL<<16):0)
               |  (Mode&CV_SPINNER1? JST_FIREL:0);
  }

  /* Autofire emulation */
  ACount=(ACount+1)&0x07;
  if(ACount>3)
  {
    if(Mode&CV_AUTOFIRER) JoyState&=~(JST_FIRER|(JST_FIRER<<16));
    if(Mode&CV_AUTOFIREL) JoyState&=~(JST_FIREL|(JST_FIREL<<16));
  }

  /* Count ticks for MIDI ouput */
  MIDITicks(1000/(Mode&CV_PAL? TMS9929_FRAMES:TMS9918_FRAMES));

  /* Flush any accumulated sound changes */
  Sync76489(&PSG,SN76489_FLUSH|(Mode&CV_DRUMS? SN76489_DRUMS:0));

  /* If exit requested, return INT_QUIT */
  if(ExitNow) return(INT_QUIT);

  /* Generate interrupt if needed */
  return(R->IRequest);
}
