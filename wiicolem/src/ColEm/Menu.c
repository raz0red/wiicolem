/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                         Menu.c                          **/
/**                                                         **/
/** This file contains runtime menu code for configuring    **/
/** the emulator. It uses console functions from Console.h. **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Coleco.h"
#include "Console.h"
#include "Sound.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CLR_BACK   PIXEL(255,255,255)
#define CLR_BACK2  PIXEL(255,200,150)
#define CLR_BACK3  PIXEL(150,255,255)
#define CLR_BACK4  PIXEL(255,255,150)
#define CLR_TEXT   PIXEL(0,0,0)
#define CLR_WHITE  PIXEL(255,255,255)
#define CLR_ERROR  PIXEL(200,0,0)

/** MenuColeco() *********************************************/
/** Invoke a menu system allowing to configure the emulator **/
/** and perform several common tasks.                       **/
/*************************************************************/
void MenuColeco(void)
{
  const char *P;
  char S[512];
  int I,J,K;

  /* Display and activate top menu */
  for(J=1;J;)
  {
    /* Compose menu */
    sprintf(S,
      "ColEm\n"
      "Load cartridge\n"
      "Load state\n"
      "Save state\n"
      "  \n"
      "PAL (Europe)     %c\n"
      "NTSC (US/Japan)  %c\n"
      "Show all sprites %c\n"
      "Select palette\n"
      "  \n"
      "Log soundtrack   %c\n"
      "Hit MIDI drums   %c\n"
      "  \n"
      "Automatic FIRE-L %c\n"
      "Automatic FIRE-R %c\n"
      "Setup spinners\n"
      "  \n"
      "Reset emulator\n"
      "Quit emulator\n"
      "  \n"
      "Done\n",
      Mode&CV_PAL?             CON_CHECK:' ',
      !(Mode&CV_PAL)?          CON_CHECK:' ',
      Mode&CV_ALLSPRITE?       CON_CHECK:' ',
      MIDILogging(MIDI_QUERY)? CON_CHECK:' ',
      Mode&CV_DRUMS?           CON_CHECK:' ',
      Mode&CV_AUTOFIREL?       CON_CHECK:' ',
      Mode&CV_AUTOFIRER?       CON_CHECK:' '
    );

    /* Replace all EOLNs with zeroes */
    for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
    /* Run menu */
    J=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK,S,J);
    /* Handle menu selection */
    switch(J)
    {
      case 1: /* Load game */
        /* Request file name */
        P=CONFile(CLR_TEXT,CLR_BACK3,".cv\0.cv.gz\0.rom\0.rom.gz\0");
        /* Try loading file, show error on failure */
        if(P&&!LoadROM(P))
          CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load file.\0\0");
        /* Exit top menu */
        J=0;
        break;

      case 2: /* Load state */
        /* Request file name */
        P=CONFile(CLR_TEXT,CLR_BACK4,".sta\0");
        /* Try loading state, show error on failure */
        if(P&&!LoadSTA(P))
          CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load state.\0\0");
        /* Exit top menu */
        J=0;
        break;

      case 3: /* Save state */
        /* Request file name */
        P=CONFile(CLR_TEXT,CLR_BACK2,".sta\0");
        /* Try saving state, show error on failure */
        if(P&&!SaveSTA(P))
          CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save state.\0\0");
        /* Exit top menu */
        J=0;
        break;

      case 5: /* PAL VDP chip */
        ResetColeco(Mode|CV_PAL);
        break;
      case 6: /* NTSC VDP chip */
        ResetColeco(Mode&~CV_PAL);
        break;
      case 7: /* Show all sprites */
        Mode^=CV_ALLSPRITE;
        VDP.MaxSprites=Mode&CV_ALLSPRITE? 255:TMS9918_MAXSPRITES; 
        break;

      case 8: /* Select palette */
        for(K=1;K;)
        {
          /* Compose menu */
          sprintf(S,
            "Select Palette\n"
            "Scaled VDP colors   %c\n"
            "Original VDP colors %c\n"
            "Faded NTSC colors   %c\n"
            "  \n"
            "Done\n",
            (Mode&CV_PALETTE)==CV_PALETTE0? CON_CHECK:' ',
            (Mode&CV_PALETTE)==CV_PALETTE1? CON_CHECK:' ',
            (Mode&CV_PALETTE)==CV_PALETTE2? CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
          /* Run menu */
          K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,K);
          /* Handle menu selection */
          switch(K)
          {
            case 1: ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE0);break;
            case 2: ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE1);break;
            case 3: ResetColeco((Mode&~CV_PALETTE)|CV_PALETTE2);break;
            case 5: K=0;break;
          }
        }
        /* Exit top menu */
        J=0;
        break;

      case 10: /* Log MIDI soundtrack */
        MIDILogging(MIDI_TOGGLE);
        break;
      case 11: /* Hit MIDI drums for noise */
        Mode^=CV_DRUMS;
        break;
      case 13: /* Autofire LEFT */
        Mode^=CV_AUTOFIREL;
        break;
      case 14: /* Autofire RIGHT */
        Mode^=CV_AUTOFIRER;
        break;

      case 15: /* Setup spinners */
        for(K=1;K;)
        {
          /* Compose menu */
          sprintf(S,
            "Setup Spinners\n"
            "X as spinner 1 %c\n"
            "X as spinner 2 %c\n"
            "Y as spinner 1 %c\n"
            "Y as spinner 2 %c\n"
            "  \n"
            "Done\n",
            (Mode&CV_SPINNER1)==CV_SPINNER1X? CON_CHECK:' ',
            (Mode&CV_SPINNER2)==CV_SPINNER2X? CON_CHECK:' ',
            (Mode&CV_SPINNER1)==CV_SPINNER1Y? CON_CHECK:' ',
            (Mode&CV_SPINNER2)==CV_SPINNER2Y? CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
          /* Run menu */
          K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,K);
          /* Handle menu selection */
          switch(K)
          {
            case 1: Mode=(Mode&~CV_SPINNER1Y)^CV_SPINNER1X;break;
            case 2: Mode=(Mode&~CV_SPINNER2Y)^CV_SPINNER2X;break;
            case 3: Mode=(Mode&~CV_SPINNER1X)^CV_SPINNER1Y;break;
            case 4: Mode=(Mode&~CV_SPINNER2X)^CV_SPINNER2Y;break;
            case 6: K=0;break;
          }
        }
        /* Exit top menu */
        J=0;
        break;

      case 17: /* Reset */
        ResetColeco(Mode);
        J=0;
        break;
      case 18: /* Quit */
        ExitNow=1;
        J=0;
        break;
      case 20: /* Done */
        J=0;
        break;
    }
  }
}
