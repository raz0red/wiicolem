/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                       AdamNet.c                         **/
/**                                                         **/
/** This file contains implementation for the AdamNet I/O   **/
/** interface found in Coleco Adam home computer.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "AdamNet.h"
#include <string.h>
#include <stdio.h>

#ifdef ANDROID
#include "EMULib.h"
#define puts   LOGI
#define printf LOGI
#endif

static word PCBAddr;
byte PCBTable[0x10000];
static byte DiskID;
static byte KBDStatus;

extern byte *ROMPage[8];    /* 8x8kB read-only (ROM) pages   */
extern byte *RAMPage[8];    /* 8x8kB read-write (RAM) pages  */
extern byte Port60;
extern int Verbose;

/** GetPCB() *************************************************/
/** Get PCB byte at given offset.                           **/
/*************************************************************/
static byte GetPCB(byte Offset)
{
  word A = (PCBAddr+Offset)&0xFFFF;
  return(ROMPage[A>>13][A&0x1FFF]);
}

/** SetPCB() *************************************************/
/** Set PCB byte at given offset.                           **/
/*************************************************************/
static void SetPCB(byte Offset,byte Value)
{
  word A = (PCBAddr+Offset)&0xFFFF;
  RAMPage[A>>13][A&0x1FFF] = Value;
}

/** IsPCB() **************************************************/
/** Return 1 if given address belongs to PCB, 0 otherwise.  **/
/*************************************************************/
static int IsPCB(word A)
{
  /* Quick check for PCB presence */
  if(!PCBTable[A]) return(0);

  /* Check if PCB is mapped in */
  if((A<0x2000) && ((Port60&0x03)!=1)) return(0);
  if((A<0x8000) && ((Port60&0x03)!=1) && ((Port60&0x03)!=3)) return(0);
  if((A>=0x8000) && (Port60&0x0C)) return(0);

  /* Check number of active devices */
  if(A>=PCBAddr+GetPCB(3)*21+4) return(0);

  /* This address belongs to AdamNet */
  return(1);
}

/** MovePCB() ************************************************/
/** Move PCB and related DCBs to a new address.             **/
/*************************************************************/
static void MovePCB(word NewAddr)
{
  int J;

  PCBTable[PCBAddr] = 0;
  PCBTable[NewAddr] = 1;

  for(J=4;J<4+15*21;J+=21)
  {
    PCBTable[(PCBAddr+J)&0xFFFF] = 0;
    PCBTable[(NewAddr+J)&0xFFFF] = 1;
  }
printf("PCB 0x%04X => 0x%04X\n",PCBAddr,NewAddr);
  PCBAddr = NewAddr;
  SetPCB(3,15);
}

static byte GetKBD()
{
  static int Cnt = 3;
  static byte Chr = 'K';

  if(Cnt) { --Cnt;return(Chr); } else return(0);
}

static void UpdateKBD(word Addr,int V)
{
  word Buf;
  int J,Count;

printf("KBD[%02Xh] = %02X\n",Addr,V);

  switch(V)
  {
    case -1:
      SetPCB(Addr,KBDStatus);
      break;
    case 1:
    case 2:
      SetPCB(Addr,KBDStatus=0x80);
      break;
    case 3:
      SetPCB(Addr,0x9B);
      KBDStatus = 0x80;
      break;
    case 4:
      Buf   = GetPCB(Addr+1) + ((word)GetPCB(Addr+2)<<8);
      Count = GetPCB(Addr+3) + ((word)GetPCB(Addr+4)<<8);
      KBDStatus = 0x80;
      SetPCB(Addr,0);
      for(J=0;J<Count;++J,Buf=(Buf+1)&0xFFFF)
      {
        V=GetKBD();
        if(V) RAMPage[Buf>>13][Buf&0x1FFF] = V==255? 0:V;
        else { KBDStatus = 0x8C;break; }
      }
      break;
  }
}

static void UpdatePRN(word Addr,int V)
{
  if(V>=0) SetPCB(Addr,V|0x80);
}

static void UpdateDSK(byte DevID,word Addr,int V)
{
  if(V>=0) SetPCB(Addr,V|0x80);
}

static void UpdateTAP(byte DevID,word Addr,int V)
{
  if(V>=0) SetPCB(Addr,V|0x80);
}

static void UpdatePCB(word Addr,int V)
{
//if(Addr!=0xFEC4)
//if(V>=0) printf("PCB[%04Xh] <= %02Xh\n",Addr,V);
//else printf("PCB[%04Xh] => %02Xh\n",Addr,GetPCB(Addr-PCBAddr));

//  if(Addr==PCBAddr+3) SetPCB(3,15);
  if((Addr==PCBAddr+4)&&(V>=0)) SetPCB(4,V|0x80);

  if((Addr!=PCBAddr)||(V<0)) return;

  switch(V)
  {
    case 1: /* Sync Z80 */
      SetPCB(0,0x80|V);
      break;
    case 2: /* Sync master 6801 */
      SetPCB(0,0x80|V);
      break;
    case 3: /* Relocate PCB */
      SetPCB(0,0x80|V);
      MovePCB(GetPCB(1)+((word)GetPCB(2)<<8));
      break;
    default:
      if(V && (V<0x80) && (Verbose&0x100))
        printf("AdamNet: Unimplemented PCB operation %02Xh\n",V);
      break;
  }
}

static void UpdateDCB(word Addr,int V)
{
  word Base;
  byte DevID;

  /* When writing, ignore invalid commands */
  if(!V || (V>=0x80)) return;

  /* Compute device ID, offset inside PCB */
  Base  = 21*((Addr-PCBAddr)/21);
  DevID = (GetPCB(Base+9)<<4) + (GetPCB(Base+16)&0x0F);
  Addr  = Addr - PCBAddr;

printf("DCB #%d at %04Xh = %02Xh\n",DevID,Addr,V);

  /* Depending on the device ID... */
  switch(DevID)
  {
    case 0x01: UpdateKBD(Addr,V);break;
    case 0x02: UpdatePRN(Addr,V);break;
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07: UpdateDSK(DiskID=DevID-4,Addr,V);break;
    case 0x08:
    case 0x09:
    case 0x18:
    case 0x19: UpdateTAP((DevID>>4)+((DevID&1)<<1),Addr,V);break;
    case 0x52: UpdateDSK(DiskID,Addr,-2);break;

    default:
      SetPCB(Addr,0x9B);
      if(Verbose&0x100)
        printf("AdamNet: %s unknown device #%d\n",V>=0? "Write to":"Read from",DevID);
      break;
  }
}

/** ReadPCB() ************************************************/
/** Read value from a given PCB or DCB address.             **/
/*************************************************************/
void ReadPCB(word A)
{
  if(IsPCB(A)) { if(A<PCBAddr+21) UpdatePCB(A,-1); else UpdateDCB(A,-1); }
}

/** WritePCB() ***********************************************/
/** Write value to a given PCB or DCB address.              **/
/*************************************************************/
void WritePCB(word A,byte V)
{
  if(IsPCB(A)) { if(A<PCBAddr+21) UpdatePCB(A,V); else UpdateDCB(A,V); }
}

/** ResetPCB() ***********************************************/
/** Reset PCB and attached hardware.                        **/
/*************************************************************/
void ResetPCB(void)
{
  /* PCB/DCB not mapped yet */
  memset(PCBTable,0,sizeof(PCBTable));

  /* Set starting PCB address */
  PCBAddr = 0x0000;
  MovePCB(0xFEC0);

  /* @@@ Reset tape and disk here */
}
