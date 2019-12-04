/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                         Coleco.h                        **/
/**                                                         **/
/** This file contains declarations relevant to the drivers **/
/** and Coleco emulation itself. See Z80.h for #defines     **/
/** related to Z80 emulation.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef COLECO_H
#define COLECO_H

#include "Z80.h"              /* Z80 CPU emulation           */
#include "SN76489.h"          /* SN76489 PSG emulation       */
#include "TMS9918.h"          /* TMS9918 VDP emulation       */
#include "AdamNet.h"          /* AdamNet I/O emulation       */
#include "AY8910.h"           /* AY8910 PSG emulation        */
#include "C24XX.h"            /* 24Cxx EEPROM emulation      */

#ifdef __cplusplus
extern "C" {
#endif

#define NORAM         0xFF    /* Byte to be returned from    */
                              /* nonexisting pages and ports */

#define MAXCHEATS     256     /* Maximal number of cheats    */
#define MAX_STASIZE   0xF000  /* Maximal state data size     */

#define CPU_CLOCK     TMS9918_CLOCK        /* Z80 clock, Hz  */
#define CPU_HPERIOD   TMS9918_LINE       /* Scanline, clocks */

/** Cheats() Arguments ***************************************/
#define CHTS_OFF      0               /* Turn all cheats off */
#define CHTS_ON       1               /* Turn all cheats on  */
#define CHTS_TOGGLE   2               /* Toggle cheats state */
#define CHTS_QUERY    3               /* Query cheats state  */

/** ResetColeco() Argument Bits ******************************/
#define CV_ADAM       0x00000001
#define CV_AUTOFIRER  0x00000002
#define CV_AUTOFIREL  0x00000004
#define CV_DRUMS      0x00000008
#define CV_NTSC       0x00000000
#define CV_PAL        0x00000010
#define CV_SPINNERS   0x000001E0
#define CV_SPINNER1   0x00000060
#define CV_SPINNER2   0x00000180
#define CV_SPINNER1X  0x00000020
#define CV_SPINNER1Y  0x00000040
#define CV_SPINNER2X  0x00000080
#define CV_SPINNER2Y  0x00000100
#define CV_PALETTE    0x00000600
#define CV_PALETTE0   0x00000000
#define CV_PALETTE1   0x00000200
#define CV_PALETTE2   0x00000400
#define CV_PALETTE3   0x00000600
#define CV_ALLSPRITE  0x00000800
#define CV_SGM        0x00001000  /* Super Game Module */
#define CV_EEPROM     0x00006000  /* Serial EEPROMs:   */
#define CV_24C08      0x00002000  /*   256-byte EEPROM */
#define CV_24C256     0x00004000  /*   32kB EEPROM     */
#define CV_SRAM       0x00008000  /* 2kB battery-backed SRAM */

#ifdef WII
extern byte *RAM;
#endif

/** Memory Areas *********************************************/
#define ROM_WRITER    (RAM)         /* 32kB SmartWriter ROM  */
#define RAM_MAIN_LO   (RAM+0x8000)  /* 32kB main Adam RAM    */
#define RAM_EXP_LO    (RAM+0x10000) /* 32kB exp Adam RAM     */
#define ROM_OS7       (RAM+0x18000) /* 8kB OS7 ROM (CV BIOS) */
#define ROM_BIOS      ROM_OS7
#define RAM_OS7       (RAM+0x1A000) /* 8x1kB main CV RAM     */
#define RAM_BASE      RAM_OS7
#define RAM_DUMMY     (RAM+0x1C000) /* 8kB dummy RAM         */
#define ROM_EOS       (RAM+0x1E000) /* 8kB EOS ROM           */
#define RAM_MAIN_HI   (RAM+0x20000) /* 32kB main Adam RAM    */
#define ROM_EXPANSION (RAM+0x28000) /* 32kB Expansion ROM    */
#define RAM_EXP_HI    (RAM+0x30000) /* 32kB exp Adam RAM     */
#define ROM_CARTRIDGE (RAM+0x38000) /* 32kB Cartridge ROM    */

/** Joystick() Result Bits ***********************************/
#define JST_NONE      0x0000
#define JST_KEYPAD    0x000F
#define JST_UP        0x0100
#define JST_RIGHT     0x0200
#define JST_DOWN      0x0400
#define JST_LEFT      0x0800
#define JST_FIRER     0x0040
#define JST_FIREL     0x4000
#define JST_0         0x0005
#define JST_1         0x0002
#define JST_2         0x0008
#define JST_3         0x0003
#define JST_4         0x000D
#define JST_5         0x000C
#define JST_6         0x0001
#define JST_7         0x000A
#define JST_8         0x000E
#define JST_9         0x0004
#define JST_STAR      0x0006
#define JST_POUND     0x0009
#define JST_PURPLE    0x0007
#define JST_BLUE      0x000B
#define JST_RED       JST_FIRER
#define JST_YELLOW    JST_FIREL
#ifdef WII
#define JST_2P_FIRER  0x00400000
#define JST_2P_FIREL  0x40000000
#endif

/******** Variables used to control emulator behavior ********/
extern byte Verbose;        /* Debug msgs ON/OFF             */
extern int  Mode;           /* Conjunction of CV_* mode bits */
extern byte UPeriod;        /* Percentage of frames to draw  */

extern int  ScrWidth;       /* Screen buffer width           */
extern int  ScrHeight;      /* Screen buffer height          */
extern void *ScrBuffer;     /* If screen buffer allocated,   */
                            /* put address here              */
/*************************************************************/

extern Z80 CPU;                       /* CPU registers+state */
extern TMS9918 VDP;                   /* TMS9918 VDP state   */
extern byte *VRAM;                    /* Video RAM           */

extern char *HomeDir;                 /* Home directory      */
extern char *SndName;                 /* Soundtrack log file */
extern char *StaName;                 /* State save file     */
extern char *SavName;                 /* EEPROM save file    */
extern char *PrnName;                 /* Printer redir. file */

extern byte ExitNow;                  /* 1: Exit emulator    */
extern byte AdamROMs;                 /* 1: Adam ROMs loaded */
extern byte PCBTable[];

/** StartColeco() ********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartColeco(const char *Cartridge);

/** TrashColeco() ********************************************/
/** Free memory allocated by StartColeco().                 **/
/*************************************************************/
void TrashColeco(void);

/** ResetColeco() ********************************************/
/** Reset CPU and hardware to new operating modes. Returns  **/
/** new value of the Mode variable (possibly != NewMode).   **/
/*************************************************************/
int ResetColeco(int NewMode);

/** MenuColeco() *********************************************/
/** Invoke a menu system allowing to configure the emulator **/
/** and perform several common tasks.                       **/
/*************************************************************/
void MenuColeco(void);

/** LoadROM() ************************************************/
/** Load given cartridge ROM file. Returns number of bytes  **/
/** on success, 0 on failure.                               **/
/*************************************************************/
#ifdef WII
int LoadROM(const char *Cartridge, const char* WiiStateFile);
#else
int LoadROM(const char *Cartridge);
#endif

/** LoadCHT() ************************************************/
/** Load cheats from .CHT file. Cheat format is either      **/
/** XXXX-XX (one byte) or XXXX-XXXX (two bytes). Returns    **/
/** the number of cheats on success, 0 on failure.          **/
/*************************************************************/
int LoadCHT(const char *Name);

/** SaveCHT() ************************************************/
/** Save cheats to a given text file. Returns the number of **/
/** cheats on success, 0 on failure.                        **/
/*************************************************************/
int SaveCHT(const char *Name);

/** LoadPAL() ************************************************/
/** Load new palette from .PAL file. Returns number of      **/
/** loaded colors on success, 0 on failure.                 **/
/*************************************************************/
int LoadPAL(const char *Name);

/** CartCRC() ************************************************/
/** Compute cartridge CRC.                                  **/
/*************************************************************/
unsigned int CartCRC(void);

/** SaveSTA() ************************************************/
/** Save emulation state to a given file. Returns 1 on      **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int SaveSTA(const char *StateFile);

/** LoadSTA() ************************************************/
/** Load emulation state from a given file. Returns 1 on    **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int LoadSTA(const char *StateFile);

/** SetScreenDepth() *****************************************/
/** Set screen depth for the display drivers. Returns 1 on  **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int SetScreenDepth(int Depth);

/** SaveState() **********************************************/
/** Save emulation state to a memory buffer. Returns size   **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int SaveState(unsigned char *Buf,unsigned int MaxSize);

/** LoadState() **********************************************/
/** Load emulation state from a memory buffer. Returns size **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int LoadState(unsigned char *Buf,unsigned int MaxSize);

/** AddCheat() ***********************************************/
/** Add a new cheat. Returns 0 on failure or the number of  **/
/** cheats on success.                                      **/
/*************************************************************/
int AddCheat(const char *Cheat);

/** DelCheat() ***********************************************/
/** Delete a cheat. Returns 0 on failure, 1 on success.     **/
/*************************************************************/
int DelCheat(const char *Cheat);

/** ResetCheats() ********************************************/
/** Remove all cheats.                                      **/
/*************************************************************/
void ResetCheats(void);

/** Cheats() *************************************************/
/** Toggle cheats on (1), off (0), inverse state (2) or     **/
/** query (3).                                              **/
/*************************************************************/
int Cheats(int Switch);

/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/
int InitMachine(void);

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/************************************ TO BE WRITTEN BY USER **/
void TrashMachine(void);

/** SetColor() ***********************************************/
/** Set color N (0..15) to (R,G,B).                         **/
/************************************ TO BE WRITTEN BY USER **/
int SetColor(byte N,byte R,byte G,byte B);

/** Joystick() ***********************************************/
/** This function is called to poll joysticks. It should    **/
/** return 2x16bit values in a following way:               **/
/**                                                         **/
/**      x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0      **/
/**                                                         **/
/** Least significant bit on the right. Active value is 1.  **/
/************************************ TO BE WRITTEN BY USER **/
unsigned int Joystick(void);

/** Mouse() **************************************************/
/** This function is called to poll mouse. It should return **/
/** the X coordinate (-512..+512) in the lower 16 bits, the **/
/** Y coordinate (-512..+512) in the next 14 bits, and the  **/
/** mouse buttons in the upper 2 bits. All values should be **/
/** counted from the screen center!                         **/
/************************************ TO BE WRITTEN BY USER **/
unsigned int Mouse(void);

#ifdef __cplusplus
}
#endif
#endif /* COLECO_H */
