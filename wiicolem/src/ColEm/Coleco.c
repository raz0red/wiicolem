/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                        Coleco.c                         **/
/**                                                         **/
/** This file contains implementation for the Coleco        **/
/** specific hardware. Initialization code is also here.    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "Coleco.h"
#include "Sound.h"
#include "CRC32.h"

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

#ifdef ANDROID
#include "MemFS.h"
#define puts   LOGI
#define printf LOGI
#endif

#ifdef ZLIB
#include <zlib.h>
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
AY8910 AYPSG;                  /* AY8910 PSG state              */
C24XX EEPROM;                  /* 24Cxx EEPROM state            */

byte *RAM;                     /* CPU address space             */
byte *ROMPage[8];              /* 8x8kB read-only (ROM) pages   */
byte *RAMPage[8];              /* 8x8kB read-write (RAM) pages  */
byte *EEPROMData;              /* 32kB EEPROM data buffer       */
byte Port20;                   /* Adam port 0x20-0x3F (AdamNet) */
byte Port60;                   /* Adam port 0x60-0x7F (memory)  */
byte Port53;                   /* SGM port 0x53 (memory)        */
byte MegaPage;                 /* Current MegaROM page          */
byte MegaSize;                 /* MegaROM size in 16kB pages    */
byte MegaCart;                 /* MegaROM page at 8000h         */
         
byte ExitNow;                  /* 1: Exit the emulator          */
byte AdamROMs;                 /* 1: All Adam ROMs are loaded   */ 

byte JoyMode;                  /* Joystick controller mode      */
unsigned int JoyState;         /* Joystick states               */
unsigned int SpinCount;        /* Spinner counters              */
unsigned int SpinStep;         /* Spinner steps                 */
unsigned int SpinState;        /* Spinner bit states            */

char *SndName    = "LOG.MID";  /* Soundtrack log file           */
char *StaName    = 0;          /* Emulation state save file     */
char *SavName    = 0;          /* EEPROM data save file         */
char *HomeDir    = 0;          /* Full path to home directory   */
char *PrnName    = 0;          /* Printer redirection file      */
FILE *PrnStream;

byte CheatsON    = 0;          /* 1: Cheats are on              */
int  CheatCount  = 0;          /* Number of cheats, <=MAXCHEATS */

/* Structure to store cheats */
struct
{
  word Addr;
  word Data;
  word Orig;
  byte Size;
  byte Text[10];
} CheatCodes[MAXCHEATS];

/* Apply RAM-based cheats */
static int ApplyCheats(void);
/* Guess some hardware modes by ROM contents */
static unsigned int GuessROM(const byte *ROM,unsigned int Size);
/* Save/load EEPROM contents */
static int SaveSAV(const char *FileName);
static int LoadSAV(const char *FileName);

#if defined(ANDROID)
#undef  feof
#define fopen           mopen
#define fclose          mclose
#define fread           mread
#define fwrite          mwrite
#define rewind          mrewind
#define fseek           mseek
#define ftell           mtell
#define fgets           mgets
#define feof            meof
#elif defined(ZLIB)
#undef  feof
#define fopen(N,M)      (FILE *)gzopen(N,M)
#define fclose(F)       gzclose((gzFile)(F))
#define fread(B,N,L,F)  gzread((gzFile)(F),B,(L)*(N))
#define fwrite(B,N,L,F) gzwrite((gzFile)(F),B,(L)*(N))
#define rewind(F)       gzrewind((gzFile)(F))
#define fseek(F,O,W)    gzseek((gzFile)(F),O,W)
#define ftell(F)        gztell((gzFile)(F))
#define fgets(B,L,F)    gzgets((gzFile)(F),B,L)
#define feof(F)         gzeof((gzFile)(F))
#endif

#ifdef WII
// The Coleco ROM
static char coleco_rom[WII_MAX_PATH] = "";
#endif

/** gethex() *************************************************/
/** Parse hexadecimal byte.                                 **/
/*************************************************************/
static byte gethex(const char *S)
{
  const char *Hex = "0123456789ABCDEF";
  const char *P;
  byte D;

  P = strchr(Hex,toupper(S[0]));
  D = P? P-Hex:0;
  P = strchr(Hex,toupper(S[1]));
  D = P? (D<<4)+(P-Hex):D;

  return(D);
}

/** MakeFileName() *******************************************/
/** Make a copy of the file name, replacing the extension.  **/
/** Returns allocated new name or 0 on failure.             **/
/*************************************************************/
char *MakeFileName(const char *FileName,const char *Extension)
{
  char *Result,*P;

  Result = malloc(strlen(FileName)+strlen(Extension)+1);
  if(!Result) return(0);

  strcpy(Result,FileName);
  if((P=strrchr(Result,'.'))) strcpy(P,Extension); else strcat(Result,Extension);
  return(Result);
}

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
  RAM        = 0;
  ExitNow    = 0;
  AdamROMs   = 0;
  StaName    = 0;
  SavName    = 0;
  MegaSize   = 2;
  MegaCart   = 0;
  EEPROMData = 0;

  /* Set up CPU modes */
  CPU.TrapBadOps = Verbose&0x04;
  CPU.IAutoReset = 1;

  /* Allocate memory for RAM and ROM */
  if(Verbose) printf("Allocating 256kB for CPU address space...");  
  if(!(RAM=malloc(0x40000))) { if(Verbose) puts("FAILED");return(0); }
  memset(RAM,NORAM,0x40000);

  /* Allocate EEPROM data buffer */
  if(Verbose) printf("Allocating 32kB for EEPROM data...");  
  if(!(EEPROMData=malloc(0x8000))) { if(Verbose) puts("FAILED");return(0); }
  memset(EEPROMData,NORAM,0x8000);  

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
    if(chdir(HomeDir))
    { if(Verbose) printf("  Failed changing to '%s' directory!\n",HomeDir); }
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
    wii_get_app_relative( "EOS.ROM", rom );
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
  if(P && Verbose) printf("%s\n",P);
  if(CurDir[0] && chdir(CurDir))
  { if(Verbose) printf("  Failed changing to '%s' directory!\n",CurDir); }
  if(P) return(0);

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
  int J,I,Size;
  char *T;
  FILE *F;

  /* Open file */
  F=fopen(Cartridge,"rb");
  if(!F) return(0);

  /* Determine size via ftell() or by reading entire [GZIPped] stream */
  if(!fseek(F,0,SEEK_END)) Size=ftell(F);
  else
  {
    /* Read file in 8kB increments */
    for(Size=0;(J=fread(RAM_DUMMY,1,0x2000,F))==0x2000;Size+=J);
    if(J>0) Size+=J;
    /* Clean up the dummy RAM! */
    memset(RAM_DUMMY,NORAM,0x2000);
  }

  /* Validate size */
  if((Size<=0)||(Size>128*0x4000)) { fclose(F);return(0); }

  /* Rewind file to the beginning */
  rewind(F);

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
  /* Rewind file to the beginning */
  rewind(F);

  /* Assume normal cartridge, not a MegaCart */
  I = 0;

  /* MegaCarts have magic number in the last 16kB page */
  if(!P&&(Size>0x8000))
  {
    if(fseek(F,(Size&~0x3FFF)-0x4000,SEEK_SET)>=0)
      if(fread(Buf,1,2,F)==2)
        if(((Buf[0]==0x55)&&(Buf[1]==0xAA))||((Buf[0]==0xAA)&&(Buf[1]==0x55)))
        {
          I = 1;
          P = ROM_CARTRIDGE;
        }

    rewind(F);
  }

  /* If ROM not recognized, drop out */
  if(!P) { fclose(F);return(0); }

  /* If loading a cartridge... */
  if(P==ROM_CARTRIDGE)
  {
    /* Pad to the nearest 16kB and find number of 16kB pages */
    Size = ((Size+0x3FFF)&~0x3FFF)>>14;

    /* Round page number up to the nearest power of two */
    for(J=2;J<Size;J<<=1);

    /* If not enough space, reallocate memory */
    if(J>MegaSize)
    {
      P = realloc(RAM,0x38000+(J<<14));
      if(!P) { fclose(F);return(0); }
      RAM = P;
      P   = ROM_CARTRIDGE;
    }

    /* Set new MegaROM size, cancel all cheats */
    Size       = J<<14;
    MegaSize   = J;
    MegaCart   = I? J-1:0;
    CheatsON   = 0;
    CheatCount = 0;
  }

  /* Rewind file to the beginning */
  rewind(F);

  /* Clear ROM buffer */
  memset(P,NORAM,Size);

  /* Read the ROM */
  J=fread(P,1,Size,F);

  /* Done with the file */
  fclose(F);

  /* Save EEPROM contents for the previous cartridge */
  if(SavName) SaveSAV(SavName);

  /* Reset hardware, guessing some hardware modes */
  ResetColeco((Mode&~(CV_EEPROM|CV_SRAM))|GuessROM(P,J));

  /* Free previous file names */
  if(StaName) free(StaName);
  if(SavName) free(SavName);

  /* Generate save file name and try loading it */
  if((SavName=MakeFileName(Cartridge,".sav"))) LoadSAV(SavName);

  /* Generate state file name and try loading it */
  if((StaName=MakeFileName(Cartridge,".sta"))) LoadSTA(StaName);

  /* Generate cheat file name and try loading it */
  if((T=MakeFileName(Cartridge,".cht"))) { LoadCHT(T);free(T); }

  /* Generate palette file name and try loading it */
  if((T=MakeFileName(Cartridge,".pal"))) { LoadPAL(T);free(T); }

  /* Done */
  return(J);
}

/** LoadCHT() ************************************************/
/** Load cheats from .CHT file. Cheat format is either      **/
/** XXXX-XX (one byte) or XXXX-XXXX (two bytes). Returns    **/
/** the number of cheats on success, 0 on failure.          **/
/*************************************************************/
int LoadCHT(const char *Name)
{
  char Buf[256],S[16];
  int Status;
  FILE *F;

  /* Open .CHT text file with cheats */
  F = fopen(Name,"rb");
  if(!F) return(0);

  /* Switch cheats off for now and remove all present cheats */
  Status = Cheats(CHTS_QUERY);
  Cheats(CHTS_OFF);
  ResetCheats();

  /* Try adding cheats loaded from file */
  while(!feof(F))
    if(fgets(Buf,sizeof(Buf),F) && (sscanf(Buf,"%9s",S)==1))
      AddCheat(S);

  /* Done with the file */
  fclose(F);

  /* Turn cheats back on, if they were on */
  Cheats(Status);

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
  return(CheatCount);
}

/** LoadPAL() ************************************************/
/** Load new palette from .PAL file. Returns number of      **/
/** loaded colors on success, 0 on failure.                 **/
/*************************************************************/
int LoadPAL(const char *Name)
{
  char S[256],*P,*T;
  FILE *F;
  int J;

  if(!(F=fopen(Name,"rb"))) return(0);

  for(J=0;(J<16)&&fgets(S,sizeof(S),F);++J)
  {
    /* Skip white space and optional '#' character */
    for(P=S;*P&&(*P<=' ');++P);
    if(*P=='#') ++P;
    /* Make sure we have got six hexadecimal digits */
    for(T=P;*T&&strchr("0123456789ABCDEF",toupper(*T));++T);
    /* If we have got six digits, parse and set color */
    if(T-P==6) VDP.XPal[J]=SetColor(J,gethex(P),gethex(P+2),gethex(P+4));
  }

  fclose(F);
  return(J);
}

/** State.h **************************************************/
/** SaveState()/LoadState(), SaveSTA()/LoadSTA() functions  **/
/** are implemented here.                                   **/
/*************************************************************/
#include "State.h"

#if defined(ZLIB) || defined(ANDROID)
#undef fopen 
#undef fclose
#undef fread 
#undef fwrite
#undef rewind
#undef fseek 
#undef ftell 
#undef fgets 
#undef feof
#endif

#if defined(ANDROID)
/* On Android, may need to open files for writing at an */
/* alternative location, if the requested location is   */
/* not available. OpenRealFile() WILL NOT USE ZLIB.     */
#define fopen OpenRealFile
#endif

/** TrashColeco() ********************************************/
/** Free memory allocated by StartColeco().                 **/
/*************************************************************/
void TrashColeco(void)
{
  /* Save EEPROM contents */
  if(SavName) SaveSAV(SavName);

  /* Free all memory and resources */
  if(RAM)        { free(RAM);RAM=0; }
  if(EEPROMData) { free(EEPROMData);EEPROMData=0; }
  if(StaName)    { free(StaName);StaName=0; }
  if(SavName)    { free(SavName);SavName=0; }

  /* Close MIDI sound log */
  TrashMIDI();

  /* Done with VDP */
  Trash9918(&VDP);
}

/** SetMemory() **********************************************/
/** Set memory pages according to passed values.            **/
/*************************************************************/
void SetMemory(byte NewPort60,byte NewPort20,byte NewPort53)
{
  byte *P;

  /* Store new values */
  Port60 = NewPort60;
  Port20 = NewPort20;
  Port53 = NewPort53;

  /* Lower 32kB ROM */
  if(Mode&CV_ADAM)
  {
    // Coleco Adam
    if(!(NewPort60&0x03)&&(NewPort20&0x02))
    {
      // EOS enabled
      ROMPage[0] = RAM_DUMMY;
      ROMPage[1] = RAM_DUMMY;
      ROMPage[2] = ROM_EOS;
      ROMPage[3] = ROM_EOS;
    }
    else if(Mode&CV_ADAM)
    {
      // Normal configuration
      ROMPage[0] = RAM+((int)(NewPort60&0x03)<<15);
      ROMPage[1] = ((NewPort60&0x03)==3? RAM_MAIN_LO:ROMPage[0])+0x2000;
      ROMPage[2] = ROMPage[1]+0x2000;
      ROMPage[3] = ROMPage[1]+0x4000;
    }
  }
  else if(Mode&CV_SGM)
  {
    // Super Game Module (SGM)
    ROMPage[0] = NewPort60&0x02? ROM_BIOS:RAM_BASE;
    if(NewPort53&0x01)
    {
      // 24kB RAM enabled
      ROMPage[1] = RAM_BASE+0x2000;
      ROMPage[2] = RAM_BASE+0x4000;
      ROMPage[3] = RAM_BASE+0x6000;
    }
    else
    {
      // Normal configuration
      ROMPage[1] = RAM_DUMMY;
      ROMPage[2] = RAM_DUMMY;
      ROMPage[3] = RAM_BASE;
    }
  }
  else
  {
    // Regular ColecoVision
    ROMPage[0] = ROM_BIOS;
    ROMPage[1] = RAM_DUMMY;
    ROMPage[2] = RAM_DUMMY;
    ROMPage[3] = RAM_BASE;
  }

  /* Upper 32kB ROM */
  P          = RAM_MAIN_HI+((int)(NewPort60&0x0C)<<13);
  ROMPage[4] = P + (MegaCart<<14);
  ROMPage[5] = ROMPage[4]+0x2000;
  ROMPage[6] = P + (((NewPort60&0x0C)==0x0C? (MegaPage&(MegaSize-1)):1)<<14);
  ROMPage[7] = ROMPage[6]+0x2000;

  /* Lower 32kB RAM */
  if(Mode&CV_ADAM)
  {
    // Coleco Adam
    if(!(NewPort60&0x03))
      RAMPage[0]=RAMPage[1]=RAMPage[2]=RAMPage[3]=RAM_DUMMY;
    else
    {
      RAMPage[0] = (NewPort60&0x03)==3? RAM_DUMMY:ROMPage[0]; 
      RAMPage[1] = ROMPage[1];
      RAMPage[2] = ROMPage[2];
      RAMPage[3] = ROMPage[3];
    }
  }
  else if(Mode&CV_SGM)
  {
    // Super Game Module (SGM)
    RAMPage[0] = NewPort60&0x02? RAM_DUMMY:ROMPage[0]; 
    RAMPage[1] = ROMPage[1];
    RAMPage[2] = ROMPage[2];
    RAMPage[3] = ROMPage[3];
  }
  else
  {
    // Regular ColecoVision
    RAMPage[0] = RAM_DUMMY; 
    RAMPage[1] = ROMPage[1];
    RAMPage[2] = ROMPage[2];
    RAMPage[3] = ROMPage[3];
  }

#ifdef WII
  if(wii_coleco_db_entry.flags&OPCODE_MEMORY)
  {
    ROMPage[1] = RAMPage[1] = RAM_MAIN_HI;
    ROMPage[2] = RAMPage[2] = RAMPage[1]+0x2000;
  }
#endif

  /* Upper 32kB RAM */
  if(NewPort60&0x04)
    RAMPage[4]=RAMPage[5]=RAMPage[6]=RAMPage[7]=RAM_DUMMY;
  else
  {
    RAMPage[4] = ROMPage[4];
    RAMPage[5] = ROMPage[5];
    RAMPage[6] = ROMPage[6];
    RAMPage[7] = ROMPage[7];
  }

  /* Reset AdamNet */
  if((Mode&CV_ADAM)&&(NewPort20==0x0F)) ResetPCB();
}

/** CartCRC() ************************************************/
/** Compute cartridge CRC.                                  **/
/*************************************************************/
unsigned int CartCRC(void)
{
  unsigned int I=0,J=0;
#ifdef WII
  // TODO: Fix this to properly calculate the CRC (ignoring SRAM)
  if(!(Mode&CV_SRAM)) {
#endif
    for(J=I=0;J<0x8000;++J) I+=ROM_CARTRIDGE[J];
#ifdef WII
  }
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

  /* Set new modes into effect and initialize hardware */
  Mode      = NewMode;
  MegaPage  = 1;
  JoyMode   = 0;
  JoyState  = 0;
  SpinState = 0;
  SpinCount = 0;
  SpinStep  = 0;

  /* Clear memory (important for NetPlay, to  */
  /* keep states at both sides consistent)    */
  /* Clearing to zeros (Heist) */
  memset(RAM_MAIN_LO,0x00,0x8000);
  memset(RAM_MAIN_HI,0x00,0x8000);
  memset(RAM_EXP_LO,0x00,0x8000);
  memset(RAM_EXP_HI,0x00,0x8000);
  memset(RAM_OS7,0x00,0x2000);

  /* Set up memory pages */
  SetMemory(Mode&CV_ADAM? 0x00:0x0F,0x00,0x00);

  /* Set scanline parameters according to video type */
  /* (this has to be done before CPU and VDP are reset) */
  VDP.MaxSprites = Mode&CV_ALLSPRITE? 255:TMS9918_MAXSPRITES; 
  VDP.Scanlines  = Mode&CV_PAL? TMS9929_LINES:TMS9918_LINES;
  CPU.IPeriod    = Mode&CV_PAL? TMS9929_LINE:TMS9918_LINE;

  /* Reset TMS9918 VDP */
  Reset9918(&VDP,ScrBuffer,ScrWidth,ScrHeight);
  /* Reset SN76489 PSG */
  Reset76489(&PSG,CPU_CLOCK,0);
  Sync76489(&PSG,SN76489_SYNC);
  /* Reset AY8910 PSG */
  Reset8910(&AYPSG,CPU_CLOCK/2, SN76489_CHANNELS);
  Sync8910(&AYPSG,AY8910_SYNC);
  /* Reset 24Cxx EEPROM */
  I = (Mode&CV_EEPROM)==CV_24C256? C24XX_24C256:C24XX_24C08;
  Reset24XX(&EEPROM,EEPROMData,I|(Verbose&0x08? C24XX_DEBUG:0));
  /* Reset Z80 CPU */
  ResetZ80(&CPU);

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
  if(Mode&CV_ADAM)
  {
    /* Write to RAM */
    RAMPage[A>>13][A&0x1FFF]=V;
    /* Adam may try writing AdamNet */
    if(PCBTable[A]) WritePCB(A,V);
  }
  else if((Mode&CV_SGM)&&(Port53&0x01))
  {
    /* Write to RAM */
    RAMPage[A>>13][A&0x1FFF]=V;
  }
  else if((A>=0x6000)&&(A<0x8000))
  {
    A&=0x03FF;
    RAM_BASE[A]       =RAM_BASE[0x0400+A]=
    RAM_BASE[0x0800+A]=RAM_BASE[0x0C00+A]=
    RAM_BASE[0x1000+A]=RAM_BASE[0x1400+A]=
    RAM_BASE[0x1800+A]=RAM_BASE[0x1C00+A]=V;
  }
  else if((A>=0xFF80)&&(MegaSize>2)&&(ROMPage[7]!=RAMPage[7]))
  {
    /* Cartridges, containing EEPROM, use [1111 1111 11xx 0000] addresses */
    if(Mode&CV_EEPROM)
    {
      switch(A)
      {
        case 0xFFC0: Write24XX(&EEPROM,EEPROM.Pins&~C24XX_SCL);break;
        case 0xFFD0: Write24XX(&EEPROM,EEPROM.Pins|C24XX_SCL);break;
        case 0xFFE0: Write24XX(&EEPROM,EEPROM.Pins&~C24XX_SDA);break;
        case 0xFFF0: Write24XX(&EEPROM,EEPROM.Pins|C24XX_SDA);break;
      }
    }

    /* MegaCarts use [1111 1111 11xx xxxx] addresses */
    if(MegaCart)
    {
      if(A>=0xFFC0)
      {
        /* Set new MegaCart ROM page at C000h */
        MegaPage   = (A-0xFFC0)&(MegaSize-1);
        ROMPage[6] = ROM_CARTRIDGE + (MegaPage<<14);
        ROMPage[7] = ROMPage[6]+0x2000;
      }
    }
    else switch(A)
    {
      case 0xFF90:
      case 0xFFA0:
      case 0xFFB0:
        /* Cartridges, containing EEPROM, use [1111 1111 10xx 0000] addresses */
        if(MegaSize==4)
        {
          MegaPage   = (A>>4)&3&(MegaSize-1);
          ROMPage[6] = ROM_CARTRIDGE + (MegaPage<<14);
          ROMPage[7] = ROMPage[6]+0x2000;
        }
        break;
      case 0xFFFF:
        /* SGM multipage ROMs write page number to FFFFh */
        MegaPage   = V&(MegaSize-1);
        ROMPage[6] = ROM_CARTRIDGE + (MegaPage<<14);
        ROMPage[7] = ROMPage[6]+0x2000;
        break;
    }
  }
  else if((A>=0xE000)&&(A<0xE800)&&(Mode&CV_SRAM))
  {
    /* SRAM at E800h..EFFFh, writable via E000h..E7FFh */
    ROMPage[A>>13][(A+0x0800)&0x1FFF] = V;
  }
  else if((wii_coleco_db_entry.flags&OPCODE_MEMORY)&&(A>=0x2000)&&(A<0x5FFF))
  {
    // To support Opcode RAM expansion
    RAMPage[A>>13][A&0x1FFF]=V;
  }
  else if(Verbose)
  {
//    printf("Illegal write RAM[%04Xh] = %02Xh\n",A,V);
  }
}

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A of Z80 address space. Copied to Z80.c and     **/
/** made inlined to speed things up.                        **/
/*************************************************************/
byte RdZ80(register word A)
{
  /* If trying to switch MegaCart... */
  if((A>=0xFFC0)&&MegaCart&&(ROMPage[7]!=RAMPage[7]))
  {
    /* Set new MegaCart ROM page */
    MegaPage   = (A-0xFFC0)&(MegaSize-1);
    ROMPage[6] = ROM_CARTRIDGE + (MegaPage<<14);
    ROMPage[7] = ROMPage[6]+0x2000;
  }
  else if((A==0xFF80)&&(MegaSize>2)&&(ROMPage[7]!=RAMPage[7])&&(Mode&CV_EEPROM))
  {
    /* Return EEPROM output bit */
    return(Read24XX(&EEPROM));
  }

  /* Adam may try reading AdamNet */
  if((Mode&CV_ADAM)&&PCBTable[A]) ReadPCB(A);

  return(ROMPage[A>>13][A&0x1FFF]);
}

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

//printf("InZ80(0x%X)\n",Port);

  switch(Port&0xE0)
  {
    case 0x40: /* Printer Status and SGM Module */
      if((Mode&CV_ADAM)&&(Port==0x40)) return(0xFF);
      if((Mode&CV_SGM)&&(Port==0x52))  return(RdData8910(&AYPSG));
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
  return(NORAM);
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write a byte to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(register word Port,register byte Value)
{
  /* Coleco uses 8bit port addressing */
  Port&=0x00FF;

//printf("OutZ80(0x%X,0x%X)\n",Port,Value);

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
      if(Mode&CV_SGM)
      {
        if(Port==0x53)      SetMemory(Port60,Port20,Value);
        else if(Port==0x50) WrCtrl8910(&AYPSG,Value);
        else if(Port==0x51) WrData8910(&AYPSG,Value);
      }
      break;

    case 0x20:
      if(Mode&CV_ADAM) SetMemory(Port60,Value,Port53);
      break;

    case 0x60:
      if(Mode&(CV_ADAM|CV_SGM)) SetMemory(Value,Port20,Port53);
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
  register int J;

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
  J = 1000/(Mode&CV_PAL? TMS9929_FRAMES:TMS9918_FRAMES);
  MIDITicks(J);
#ifdef WII
  //Loop8910(&AYPSG,J*1000);
#endif  

  /* Flush any accumulated sound changes */
  Sync76489(&PSG,SN76489_FLUSH|(Mode&CV_DRUMS? SN76489_DRUMS:0));
  Sync8910(&AYPSG,AY8910_FLUSH|(Mode&CV_DRUMS? AY8910_DRUMS:0));

  /* Apply RAM-based cheats */
  if(CheatsON&&CheatCount) ApplyCheats();

  /* If exit requested, return INT_QUIT */
  if(ExitNow) return(INT_QUIT);

  /* Generate interrupt if needed */
  return(R->IRequest);
}

/** SaveCHT() ************************************************/
/** Save cheats to a given text file. Returns the number of **/
/** cheats on success, 0 on failure.                        **/
/*************************************************************/
int SaveCHT(const char *Name)
{
  FILE *F;
  int J;

  /* Open .CHT text file with cheats */
  F = fopen(Name,"wb");
  if(!F) return(0);

  /* Save cheats */
  for(J=0;J<CheatCount;++J)
    fprintf(F,"%s\n",CheatCodes[J].Text);

  /* Done */
  fclose(F);
  return(CheatCount);
}

/** AddCheat() ***********************************************/
/** Add a new cheat. Returns 0 on failure or the number of  **/
/** cheats on success.                                      **/
/*************************************************************/
int AddCheat(const char *Cheat)
{
  static const char *Hex = "0123456789ABCDEF";
  unsigned int D;
  char *P;
  int J,N;

  /* Table full: no more cheats */
  if(CheatCount>=MAXCHEATS) return(0);

  /* Check cheat length and decode */
  N=strlen(Cheat);
  if((N!=9)&&(N!=7)) return(0);
  for(J=0,D=0;J<N;J++)
    if(J==4) { if(Cheat[J]!='-') return(0); }
    else
    {
      P=strchr(Hex,toupper(Cheat[J]));
      if(!P) return(0); else D=(D<<4)|(P-Hex);
    }

  /* Add cheat */
  strcpy((char *)CheatCodes[CheatCount].Text,Cheat);
  if(N==9)
  {
    CheatCodes[CheatCount].Addr = D>>16;
    CheatCodes[CheatCount].Data = D&0xFFFF;
    CheatCodes[CheatCount].Size = 2;
  }
  else
  {
    CheatCodes[CheatCount].Addr = D>>8;
    CheatCodes[CheatCount].Data = D&0xFF;
    CheatCodes[CheatCount].Size = 1;
  }

  /* Successfully added a cheat! */
  return(++CheatCount);
}

/** DelCheat() ***********************************************/
/** Delete a cheat. Returns 0 on failure, 1 on success.     **/
/*************************************************************/
int DelCheat(const char *Cheat)
{
  int I,J;

  /* Scan all cheats */
  for(J=0;J<CheatCount;++J)
  {
    /* Match cheat text */
    for(I=0;Cheat[I]&&CheatCodes[J].Text[I];++I)
      if(CheatCodes[J].Text[I]!=toupper(Cheat[I])) break;
    /* If cheat found... */
    if(!Cheat[I]&&!CheatCodes[J].Text[I])
    {
      /* Shift cheats by one */
      if(--CheatCount!=J)
        memcpy(&CheatCodes[J],&CheatCodes[J+1],(CheatCount-J)*sizeof(CheatCodes[0]));
      /* Cheat deleted */
      return(1);
    }
  }

  /* Cheat not found */
  return(0);
}

/** ResetCheats() ********************************************/
/** Remove all cheats.                                      **/
/*************************************************************/
void ResetCheats(void) { Cheats(CHTS_OFF);CheatCount=0; }

/** ApplyCheats() ********************************************/
/** Apply RAM-based cheats. Returns the number of applied   **/
/** cheats.                                                 **/
/*************************************************************/
int ApplyCheats(void)
{
  int J,I;

  /* For all current cheats that fall into RAM address range... */
  for(J=I=0;J<CheatCount;++J)
    if((CheatCodes[J].Addr>=0x6000)&&(CheatCodes[J].Addr<0x6400))
    {
      WrZ80(CheatCodes[J].Addr,CheatCodes[J].Data&0xFF);
      if(CheatCodes[J].Size>1)
        WrZ80(CheatCodes[J].Addr+1,CheatCodes[J].Data>>8);
      ++I;
    }

  /* Return number of applied cheats */
  return(I);
}

/** Cheats() *************************************************/
/** Toggle cheats on (1), off (0), inverse state (2) or     **/
/** query (3).                                              **/
/*************************************************************/
int Cheats(int Switch)
{
  int J,Size;
  byte *P;

  switch(Switch)
  {
    case CHTS_ON:
    case CHTS_OFF:    if(Switch==CheatsON) return(CheatsON);
    case CHTS_TOGGLE: Switch=!CheatsON;break;
    default:          return(CheatsON);
  }

  /* Compute total ROM size */
  Size = MegaSize<<14;

  /* If toggling cheats... */
  if(Switch!=CheatsON)
  {
    /* If enabling cheats... */
    if(Switch)
    {
      /* Patch ROM with the cheat values */
      for(J=0;J<CheatCount;++J)
        if((CheatCodes[J].Addr<0x6000)||(CheatCodes[J].Addr>=0x6400))
          if(CheatCodes[J].Addr+CheatCodes[J].Size<=Size)
          {
            P = ROM_CARTRIDGE + CheatCodes[J].Addr;
            CheatCodes[J].Orig = P[0];
            P[0] = CheatCodes[J].Data;
            if(CheatCodes[J].Size>1)
            {
              CheatCodes[J].Orig |= (int)P[1]<<8;
              P[1] = CheatCodes[J].Data>>8;
            }
          }
    }
    else
    {
      /* Restore original ROM values */
      for(J=0;J<CheatCount;++J)
        if((CheatCodes[J].Addr<0x6000)||(CheatCodes[J].Addr>=0x6400))
          if(CheatCodes[J].Addr+CheatCodes[J].Size<=Size)
          {
            P = ROM_CARTRIDGE + CheatCodes[J].Addr;
            P[0] = CheatCodes[J].Orig;
            if(CheatCodes[J].Size>1)
              P[1] = CheatCodes[J].Orig>>8;
          }
    }

    /* Done toggling cheats */
    CheatsON = Switch;
  }

  /* Done */
  if(Verbose) printf("Cheats %s\n",CheatsON? "ON":"OFF");
  return(CheatsON);
}

/** SaveSAV() ************************************************/
/** Save EEPROM memory contents to a given file. Returns 1  **/
/** on success, 0 on failure.                               **/
/*************************************************************/
int SaveSAV(const char *FileName)
{
  unsigned int Size = Size24XX(&EEPROM);
  unsigned int J;
  byte *P = EEPROMData;
  FILE *F;

  /* If no EEPROM, see if we have 2kB SRAM */
  if(!P||!Size) { if(!(Mode&CV_SRAM)) return(0); else { P=ROMPage[7]+0x800;Size=0x800; } }

  /* Check if EEPROM has data */
  for(J=0;(J<Size)&&(P[J]==NORAM);++J);

  /* If EEPROM or SRAM has no data... */
  if(J==Size)
  {
    /* Do not save file if it does not exist yet */
    if(!(F=fopen(FileName,"rb"))) return(1); else fclose(F);
  }

  /* Must have file */
  if(!(F=fopen(FileName,"wb"))) return(0);

  /* Save */
  J = fwrite(P,1,Size,F);

  /* Done */
  fclose(F);
  return(J==Size);
}

/** LoadSAV() ************************************************/
/** Load EEPROM memory contents from a given file. Returns  **/
/** 1 on success, 0 on failure.                             **/
/*************************************************************/
int LoadSAV(const char *FileName)
{
  unsigned int Size = Size24XX(&EEPROM);
  unsigned int J;
  byte *P = EEPROMData;
  FILE *F;

  /* If no EEPROM, see if we have 2kB SRAM */
  if(!P||!Size) { if(!(Mode&CV_SRAM)) return(0); else { P=ROMPage[7]+0x800;Size=0x800; } }

  /* Must have file */
  if(!(F=fopen(FileName,"rb"))) return(0);

  /* Load */
  J = fread(P,1,Size,F);

  /* Done */
  fclose(F);
  return(J==Size);
}

/** GuessROM() ***********************************************/
/** Guess some emulation modes by ROM CRC. Returns sum of   **/
/** guessed bits.                                           **/
/*************************************************************/
unsigned int GuessROM(const byte *ROM,unsigned int Size)
{
  static struct { const char *Name;unsigned int CRC,Mode; } Games[] =
  {
    { "Boxxle",             0x62DACF07,CV_24C256 }, /* 32kB EEPROM */
    { "Black Onyx",         0xDDDD1396,CV_24C08  }, /* 256-byte EEPROM */
    { "Lord Of The Dungeon",0xFEE15196,CV_SRAM   }, /* 24kB ROM + 8kB garbage + 2kB SRAM */
    { "Lord Of The Dungeon",0x1053F610,CV_SRAM   }, /* 24kB ROM + 2kB SRAM */
    { 0,0,0 }
  };
  unsigned int CRC,J;
  unsigned int Guess;

  /* Nothing guessed yet */
  Guess = 0;

  /* Compute game CRC */
  CRC = ComputeCRC32(0,ROM,Size);

  /* Find game by CRC */
  for(J=0;Games[J].Mode;++J)
    if(CRC==Games[J].CRC) { Guess=Games[J].Mode;break; }

  if(Verbose)
  {
    if(Games[J].Mode) printf("identified as %s...",Games[J].Name);
    else printf("CRC=%08Xh...",CRC);
  }

  /* Return whatever has been guessed */
  return(Guess);
}
