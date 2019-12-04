/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                          ColEm.c                        **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "Coleco.h"
#include "EMULib.h"
#include "Help.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

char *Options[]=
{ 
  "verbose","pal","ntsc","skip","help",
  "adam","cv","sgm","nosgm","24c08","24c256","sram",
  "allspr","autoa","noautoa","autob","noautob",
  "spin1x","spin1y","spin2x","spin2y",
  "drums","nodrums","logsnd","palette",
  "home","sound","nosound","trap",
  "sync","nosync","scale","vsync",
  0
};

extern const char *Title;/* Program title                     */
extern int   UseSound;   /* Sound mode                        */
extern int   UseZoom;    /* Scaling factor (UNIX)             */
extern int   UseEffects; /* EFF_* bits (UNIX/MSDOS)           */
extern int   SyncFreq;   /* Sync screen updates (UNIX/MSDOS)  */

/** main() ***************************************************/
/** This is a main() function used in Unix and MSDOS ports. **/
/** It parses command line arguments, sets emulation        **/
/** parameters, and passes control to the emulation itself. **/
/*************************************************************/
int main(int argc,char *argv[])
{
  char *CartName="CART.ROM";
  char *P;
  int N,J,I;

#if defined(DEBUG)
  CPU.Trap=0xFFFF;
  CPU.Trace=0;
#endif

#if defined(MSDOS)
  Verbose=0;
#else
  Verbose=5;
#endif

  /* Set home directory to where executable is */
#if defined(MSDOS) || defined(WINDOWS)
  P=strrchr(argv[0],'\\');
#else
  P=strrchr(argv[0],'/');
#endif
  if(P) { *P='\0';HomeDir=argv[0]; }

#if defined(UNIX) || defined(MAEMO) || defined(MSDOS)
  /* Extract visual effects arguments */
  UseEffects = ParseEffects(argv,UseEffects);
#endif

  for(N=1,I=0;(N<argc)&&argv[N];N++)
    if(*argv[N]!='-')
      switch(I++)
      {
        case 0: CartName=argv[N];break;
        default: printf("%s: Excessive filename '%s'\n",argv[0],argv[N]);
      }
    else
    {    
      for(J=0;Options[J];J++)
        if(!strcmp(argv[N]+1,Options[J])) break;
      switch(J)
      {
        case 0:  N++;
                 if(N<argc) Verbose=atoi(argv[N]);
                 else printf("%s: No verbose level supplied\n",argv[0]);
                 break;
        case 1:  Mode|=CV_PAL;break;
        case 2:  Mode&=~CV_PAL;break;
  	case 3:  N++;
                 if(N>=argc)
                   printf("%s: No skipped frames percentage supplied\n",argv[0]);
                 else
                 {
                   J=atoi(argv[N]);
                   if((J>=0)&&(J<=99)) UPeriod=100-J; 
                 }
                 break;
	case 4:  printf
                 ("%s by Marat Fayzullin    (C)FMS 1994-2019\n",Title);
                 for(J=0;HelpText[J];J++) puts(HelpText[J]);
                 return(0);
        case 5:  Mode|=CV_ADAM;break;
        case 6:  Mode&=~CV_ADAM;break;
        case 7:  Mode|=CV_SGM;break;
        case 8:  Mode&=~CV_SGM;break;
        case 9:  Mode=(Mode&~CV_EEPROM)|CV_24C08;break;
        case 10: Mode=(Mode&~CV_EEPROM)|CV_24C256;break;
        case 11: Mode|=CV_SRAM;break;
        case 12: Mode|=CV_ALLSPRITE;break;
        case 13: Mode|=CV_AUTOFIRER;break;
        case 14: Mode&=~CV_AUTOFIRER;break;
        case 15: Mode|=CV_AUTOFIREL;break;
        case 16: Mode&=~CV_AUTOFIREL;break;
        case 17: Mode|=CV_SPINNER1X;break;
        case 18: Mode|=CV_SPINNER1Y;break;
        case 19: Mode|=CV_SPINNER2X;break;
        case 20: Mode|=CV_SPINNER2Y;break;
        case 21: Mode|=CV_DRUMS;break;
        case 22: Mode&=~CV_DRUMS;break;
        case 23: N++;
                 if(N<argc) SndName=argv[N];
                 else printf("%s: No file for soundtrack logging\n",argv[0]);
                 break;
        case 24: N++;
                 if(N>=argc)
                   printf("%s: No palette number supplied\n",argv[0]);
                 else
                 {
                   J    = atoi(argv[N]);
                   J    = J==1? CV_PALETTE1
                        : J==2? CV_PALETTE2
                        :       CV_PALETTE0;
                   Mode = (Mode&~CV_PALETTE)|J;
                 }
                 break;
        case 25: N++;
                 if(N<argc) HomeDir=argv[N];
                 else printf("%s: No home directory supplied\n",argv[0]);
                 break;
        case 26: N++;
                 if(N>=argc) { UseSound=1;N--; }
                 else if(sscanf(argv[N],"%d",&UseSound)!=1)
                      { UseSound=1;N--; }
                 break;
        case 27: UseSound=0;break;

#if defined(DEBUG)
        case 28: N++;
                 if(N>=argc)
                   printf("%s: No trap address supplied\n",argv[0]);
                 else
                   if(!strcmp(argv[N],"now")) CPU.Trace=1;
                   else sscanf(argv[N],"%hX",&CPU.Trap);
                 break;
#endif /* DEBUG */

#if defined(UNIX) || defined(MSDOS) || defined(MAEMO)
        case 29: N++;
                 if(N<argc) SyncFreq=atoi(argv[N]);
                 else printf("%s: No sync frequency supplied\n",argv[0]);
                 break;
        case 30: SyncFreq=0;break;
#endif /* UNIX || MSDOS || MAEMO */

#if defined(UNIX)
        case 31: N++;
                 if(N>=argc) { UseZoom=1;N--; }
                 else if(sscanf(argv[N],"%d",&UseZoom)!=1)
                      { UseZoom=1;N--; }
                 break;
#endif /* UNIX */

#if defined(MSDOS)
        case 32: SyncFreq=-1;break;
#endif /* MSDOS */

        default: printf("%s: Wrong option '%s'\n",argv[0],argv[N]);
      }
    }

  if(!InitMachine()) return(1);
  StartColeco(CartName);
  TrashColeco();
  TrashMachine();
  return(0);
}
