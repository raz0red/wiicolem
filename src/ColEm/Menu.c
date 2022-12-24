/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                         Menu.c                          **/
/**                                                         **/
/** This file contains runtime menu code for configuring    **/
/** the emulator. It uses console functions from Console.h. **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Console.h"
#include "Sound.h"
#include "Coleco.h"
#include "Hunt.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CLR_BACK   PIXEL(255,255,255)
#define CLR_BACK2  PIXEL(255,200,150)
#define CLR_BACK3  PIXEL(150,255,255)
#define CLR_BACK4  PIXEL(255,255,150)
#define CLR_BACK5  PIXEL(255,150,255)
#define CLR_BACK6  PIXEL(150,255,150)
#define CLR_TEXT   PIXEL(0,0,0)
#define CLR_WHITE  PIXEL(255,255,255)
#define CLR_ERROR  PIXEL(200,0,0)
#define CLR_INFO   PIXEL(0,128,0)

/** Cheat Structures *****************************************/
extern int CheatCount;      /* # of cheats in the Cheats[]   */
extern struct { word Addr,Data,Orig;byte Size,Text[10]; } CheatCodes[MAXCHEATS];

static char SndNameBuf[256];

/** MenuColeco() *********************************************/
/** Invoke a menu system allowing to configure the emulator **/
/** and perform several common tasks.                       **/
/*************************************************************/
void MenuColeco(void)
{
  const char *P;
  char *PP,*T,S[512];
  int I,J,K,V,M;

  /* Display and activate top menu */
  for(J=1;J;)
  {
    /* Compose menu */
    sprintf(S,
      "ColEm\n"
      "Load file\n"
      "Save file\n"
      "  \n"
      "Hardware model\n"
      "Video options\n"
      "Input options\n"
      "Disks & tapes\n"
      "  \n"
      "Log soundtrack   %c\n"
      "Hit MIDI drums   %c\n"
      "  \n"
      "Cheats\n"
      "Search cheats\n"
      "  \n"
      "Reset emulator\n"
      "Quit emulator\n"
      "  \n"
      "Done\n",
      MIDILogging(MIDI_QUERY)? CON_CHECK:' ',
      Mode&CV_DRUMS?           CON_CHECK:' '
    );

    /* Replace all EOLNs with zeroes */
    for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
    /* Run menu */
    J=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK,S,J);
    /* Handle menu selection */
    switch(J)
    {
      case 1: /* Load file */
        /* Request file name */
        P=CONFile(CLR_TEXT,CLR_BACK3,".rom\0.cv\0.col\0.bin\0.dsk\0.fdi\0.ddp\0.sta\0.rom.gz\0.cv.gz\0.col.gz\0.bin.gz\0.dsk.gz\0.fdi.gz\0.ddp.gz\0.sta.gz\0.pal\0.cht\0");
        /* Try loading file, show error on failure */
        if(P&&!LoadFile(P))
          CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load file.\0\0");
        /* Exit top menu */
        J=0;
        break;

      case 2: /* Save file */
        /* Run menu */
        switch(CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,
          "Save File\0Emulation state\0Printer output\0MIDI soundtrack\0",1
        ))
        {
          case 1: /* Save state */
        /* Request file name */
        P=CONFile(CLR_TEXT,CLR_BACK2,".sta\0");
        /* Try saving state, show error on failure */
        if(P&&!SaveSTA(P))
          CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save state.\0\0");
            break;
          case 2: /* Printer output file */
            /* Request file name */
            P=CONFile(CLR_TEXT,CLR_BACK2,".prn\0.out\0.txt\0");
            /* Try changing printer output */
            if(P) ChangePrinter(P);
            break;
          case 3: /* Soundtrack output file */
            /* Request file name */
            P=CONFile(CLR_TEXT,CLR_BACK2,".mid\0.rmi\0");
            if(P)
            {
              /* Try changing MIDI log output, show error on failure */
              if(strlen(P)+1>sizeof(SndNameBuf))
                CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Name too long.\0\0");
              else
              {
                strcpy(SndNameBuf,P);
                SndName=SndNameBuf;
                InitMIDI(SndName);
                MIDILogging(MIDI_ON);
              }
            }
            break;
        }
        /* Exit top menu */
        J=0;
        break;

      case 4: /* Hardware model */
        for(K=1;K;)
        {
          /* Compose menu */
          sprintf(S,
            "Hardware Model\n"
            "Coleco Adam       %c\n"
            "ColecoVision      %c\n"
            "Super Game Module %c\n"
            "  \n"
            "No EEPROM         %c\n"
            "24c08 EEPROM      %c\n"
            "24c256 EEPROM     %c\n"
            "SRAM & battery    %c\n"
            "  \n"
            "Done\n",
            Mode&CV_ADAM?                CON_CHECK:' ',
            !(Mode&CV_ADAM)?             CON_CHECK:' ',
            Mode&CV_SGM?                 CON_CHECK:' ',
            (Mode&CV_EEPROM)==0?         CON_CHECK:' ',
            (Mode&CV_EEPROM)==CV_24C08?  CON_CHECK:' ',
            (Mode&CV_EEPROM)==CV_24C256? CON_CHECK:' ',
            Mode&CV_SRAM?                CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
          /* Run menu */
          K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK6,S,K);
          /* Handle menu selection */
          switch(K)
          {
            case 1: ResetColeco(Mode|CV_ADAM);break;
            case 2: ResetColeco(Mode&~CV_ADAM);break;
            case 3: ResetColeco(Mode^CV_SGM);break;
            case 5: ResetColeco(Mode&~CV_EEPROM);break;
            case 6: ResetColeco((Mode&~CV_EEPROM)|CV_24C08);break;
            case 7: ResetColeco((Mode&~CV_EEPROM)|CV_24C256);break;
            case 8: ResetColeco(Mode^CV_SRAM);break;
            case 10: K=0;break;
          }
        }
        /* Exit top menu */
        J=0;
        break;

      case 5: /* Video options */
        for(K=1;K;)
        {
          /* Compose menu */
          sprintf(S,
            "Video Options\n"
            "PAL (Europe)        %c\n"
            "NTSC (US/Japan)     %c\n"
            "  \n"
            "Show all sprites    %c\n"
            "  \n"
            "Scaled VDP colors   %c\n"
            "Original VDP colors %c\n"
            "Faded NTSC colors   %c\n"
            "  \n"
            "Done\n",
            Mode&CV_PAL?                    CON_CHECK:' ',
            !(Mode&CV_PAL)?                 CON_CHECK:' ',
            Mode&CV_ALLSPRITE?              CON_CHECK:' ',
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
            case 1: ResetColeco(Mode|CV_PAL);break;
            case 2: ResetColeco(Mode&~CV_PAL);break;
            case 4: /* Show all sprites */
              Mode^=CV_ALLSPRITE;
              VDP.MaxSprites=Mode&CV_ALLSPRITE? 255:TMS9918_MAXSPRITES; 
              break;
            case 6: ChangePalette(CV_PALETTE0);break;
            case 7: ChangePalette(CV_PALETTE1);break;
            case 8: ChangePalette(CV_PALETTE2);break;
            case 10: K=0;break;
          }
        }
        /* Exit top menu */
        J=0;
        break;

      case 6: /* Input options */
        for(K=1;K;)
        {
          /* Compose menu */
          sprintf(S,
            "Input Options\n"
            "Automatic FIRE-L %c\n"
            "Automatic FIRE-R %c\n"
            "  \n"
            "X as spinner #1  %c\n"
            "X as spinner #2  %c\n"
            "Y as spinner #1  %c\n"
            "Y as spinner #2  %c\n"
            "  \n"
            "Done\n",
            Mode&CV_AUTOFIREL?                CON_CHECK:' ',
            Mode&CV_AUTOFIRER?                CON_CHECK:' ',
            (Mode&CV_SPINNER1)==CV_SPINNER1X? CON_CHECK:' ',
            (Mode&CV_SPINNER2)==CV_SPINNER2X? CON_CHECK:' ',
            (Mode&CV_SPINNER1)==CV_SPINNER1Y? CON_CHECK:' ',
            (Mode&CV_SPINNER2)==CV_SPINNER2Y? CON_CHECK:' '
          );
          /* Replace all EOLNs with zeroes */
          for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
          /* Run menu */
          K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK3,S,K);
          /* Handle menu selection */
          switch(K)
          {
            case 1: Mode^=CV_AUTOFIREL;break;
            case 2: Mode^=CV_AUTOFIRER;break;
            case 4: Mode=(Mode&~CV_SPINNER1Y)^CV_SPINNER1X;break;
            case 5: Mode=(Mode&~CV_SPINNER2Y)^CV_SPINNER2X;break;
            case 6: Mode=(Mode&~CV_SPINNER1X)^CV_SPINNER1Y;break;
            case 7: Mode=(Mode&~CV_SPINNER2X)^CV_SPINNER2Y;break;
            case 9: K=0;break;
          }
        }
        /* Exit top menu */
        J=0;
        break;

      case 7: /* Disks & tapes */
        /* Compose menu */
        sprintf(S,
          "Disks & Tapes\n"
          "Disk drive A:\n"
          "Disk drive B:\n"
          "Disk drive C:\n"
          "Disk drive D:\n"
          "  \n"
          "Tape drive A:\n"
          "Tape drive B:\n"
          "Tape drive C:\n"
          "Tape drive D:\n"
          "  \n"
          "Done\n"
        );
        /* Replace all EOLNs with zeroes */
        for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
        /* Run menu */
        K=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK4,S,1);
          /* Handle menu selection */
          switch(K)
          {
          case 1: case 2: case 3: case 4:
            /* Compute drive number */
            K -= 1;
            /* Create disk operations menu */
            sprintf(S,
              "Disk Drive %c:\n"
              "Load disk\n"
              "New disk\n"
              "Eject disk\n"
              " \n"
              "Save DSK image\n"
              "Save FDI image\n",
              'A'+K
            );
            /* Replace all EOLNs with zeroes */
            for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
            /* Run menu and handle menu selection */
            switch(CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK2,S,1))
            {
              case 1: /* Load disk */
                P=CONFile(CLR_TEXT,CLR_BACK3,".dsk\0.dsk.gz\0.fdi\0.fdi.gz\0");
                if(P&&!ChangeDisk(K,P))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load disk image.\0\0");
                break;
              case 2: /* New disk */
                ChangeDisk(K,"");
                break;
              case 3: /* Eject disk */
                ChangeDisk(K,0);
                break;
              case 5: /* Save .DSK image */
                P=CONFile(CLR_TEXT,CLR_BACK2,".dsk\0");
                if(P&&!SaveFDI(&Disks[K],P,FMT_ADMDSK))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save disk image.\0\0");
                break;
              case 6: /* Save .FDI image */
                P=CONFile(CLR_TEXT,CLR_BACK2,".fdi\0");
                if(P&&!SaveFDI(&Disks[K],P,FMT_FDI))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save disk image.\0\0");
                break;
          }
            break;

          case 6: case 7: case 8: case 9:
            /* Compute tape number */
            K -= 6;
            /* Create tape operations menu */
            sprintf(S,
              "Tape Drive %c:\n"
              "Load tape\n"
              "New tape\n"
              "Eject tape\n"
              " \n"
              "Save DDP image\n"
              "Save FDI image\n",
              'A'+K
            );
            /* Replace all EOLNs with zeroes */
            for(I=0;S[I];I++) if(S[I]=='\n') S[I]='\0';
            /* Run menu and handle menu selection */
            switch(CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK2,S,1))
            {
              case 1: /* Load tape */
                P=CONFile(CLR_TEXT,CLR_BACK3,".ddp\0.ddp.gz\0.fdi\0.fdi.gz\0");
                if(P&&!ChangeTape(K,P))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot load tape image.\0\0");
                break;
              case 2: /* New tape */
                ChangeTape(K,"");
                break;
              case 3: /* Eject tape */
                ChangeTape(K,0);
                break;
              case 5: /* Save .DDP image */
                P=CONFile(CLR_TEXT,CLR_BACK2,".ddp\0");
                if(P&&!SaveFDI(&Tapes[K],P,FMT_DDP))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save tape image.\0\0");
                break;
              case 6: /* Save .FDI image */
                P=CONFile(CLR_TEXT,CLR_BACK2,".fdi\0");
                if(P&&!SaveFDI(&Tapes[K],P,FMT_FDI))
                  CONMsg(-1,-1,-1,-1,CLR_BACK,CLR_ERROR,"Error","Cannot save tape image.\0\0");
                break;
            }
            break;
        }
        /* Exit top menu */
        J=0;
        break;

      case 9: /* Log MIDI soundtrack */
        MIDILogging(MIDI_TOGGLE);
        break;
      case 10: /* Hit MIDI drums for noise */
        Mode^=CV_DRUMS;
        break;

      case 12: /* Cheats */
        /* Allocate buffer for cheats */
        PP=malloc(MAXCHEATS*2*16+64);
        if(!PP) break;
        /* Save cheat setting and turn cheats off */
        K=Cheats(CHTS_QUERY);
        Cheats(CHTS_OFF);
        /* Menu loop */
        for(I=1;I;)
        {
          /* Compose menu */
          sprintf(PP,
            "Cheat Codes\n"
            "Enabled   %c\n"
            "New cheat\n"
            "Done\n"
            " \n",
            K? CON_CHECK:' '
          );
          T=PP+strlen(PP);
          for(J=0;J<CheatCount;++J)
          { strcpy(T,(const char *)CheatCodes[J].Text);T+=strlen(T);*T++='\n'; }
          *T='\0';

          /* Replace all EOLNs with zeroes */
          for(J=0;PP[J];J++) if(PP[J]=='\n') PP[J]='\0';
          /* Run menu */
          I=CONMenu(-1,-1,-1,24,CLR_TEXT,CLR_BACK4,PP,I);
          /* Handle menu selection */
          switch(I)
          {
            case 1:
              K=!K;
              break;
            case 2:
              T=CONInput(-1,-1,CLR_TEXT,CLR_BACK2,"New cheat:",S,10);
              if(T) AddCheat(T);
              break;
            case 3:
              I=0;
              break;
            default:
              /* No cheats above this line */
              if(I<5) break;
              /* Find cheat */
              for(T=PP,J=0;*T&&(J<I);++J) T+=strlen(T)+1;
              /* Delete cheat */
              if(T) DelCheat(T);
              break;
          }
        }
        /* Put cheat settings into effect */
        if(K) Cheats(CHTS_ON);
        /* Done */
        free(PP);
        J=0;
        break;

      case 13: /* Hunt for cheat codes */
        /* Until user quits the menu... */
        for(I=1;I;)
        {
          /* Compose menu */
          sprintf(S,
            "Cheat Hunter\n"
            "Clear all watches\n"
            "Add a new watch\n"
            "Scan watches\n"
            "See cheat codes\n"
            "  \n"
            "Done\n"
          );

          /* Replace all EOLNs with zeroes */
          for(J=0;S[J];J++) if(S[J]=='\n') S[J]='\0';

          /* Run menu */
          I=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK5,S,I);

          /* Handle menu selection */
          switch(I)
          {
            case 1: InitHUNT();break;
            case 6: I=0;break;

            case 2:
              /* Ask for search value in 0..65535 range */
              for(K=-1;(K<0)&&(P=CONInput(-1,-1,CLR_TEXT,CLR_BACK4,"Watch Value",S,6|CON_DEC));)
              {
                K = strtoul(P,0,10);
                K = K<0x10000? K:-1;
              }

              /* If cancelled, drop out */
              if(!P) { I=0;break; }

              /* Ask for search options */
              for(I=1,V=K,M=0;I;)
              {
                /* Force 16bit mode for large values */
                if((K>=0x100)||(V>=0x100)) M|=HUNT_16BIT;

                sprintf(S,
                  "Search for %d\n"
                  "New value %5d\n"
                  "  \n"
                  "8bit value    %c\n"
                  "16bit value   %c\n"
                  "  \n"
                  "Constant      %c\n"
                  "Changes by +1 %c\n"
                  "Changes by -1 %c\n"
                  "Changes by +N %c\n"
                  "Changes by -N %c\n"
                  "  \n"
                  "Search now\n",
                  K,V,
                  !(M&HUNT_16BIT)? CON_CHECK:' ',
                  M&HUNT_16BIT?    CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_CONSTANT?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_PLUSONE?   CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_MINUSONE?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_PLUSMANY?  CON_CHECK:' ',
                  (M&HUNT_MASK_CHANGE)==HUNT_MINUSMANY? CON_CHECK:' '
                );

                /* Replace all EOLNs with zeroes */
                for(J=0;S[J];J++) if(S[J]=='\n') S[J]='\0';

                /* Run menu */
                I=CONMenu(-1,-1,-1,-1,CLR_TEXT,CLR_BACK2,S,I);

                /* Change options */
                switch(I)
                {
                  case 1:
                    /* Ask for replacement value in 0..65535 range */
                    P  = CONInput(-1,-1,CLR_TEXT,CLR_BACK4,"New Value",S,6|CON_DEC);
                    I  = P? strtoul(P,0,10):-1;
                    V = (I>=0)&&(I<0x10000)? I:V;
                    I  = 1;
                    break;
                  case 3:  M&=~HUNT_16BIT;break;
                  case 4:  M|=HUNT_16BIT;break;
                  case 6:  M=(M&~HUNT_MASK_CHANGE)|HUNT_CONSTANT;break;
                  case 7:  M=(M&~HUNT_MASK_CHANGE)|HUNT_PLUSONE;break;
                  case 8:  M=(M&~HUNT_MASK_CHANGE)|HUNT_MINUSONE;break;
                  case 9:  M=(M&~HUNT_MASK_CHANGE)|HUNT_PLUSMANY;break;
                  case 10: M=(M&~HUNT_MASK_CHANGE)|HUNT_MINUSMANY;break;
                  case 12:
                    /* Search for value RAM */
                    J = AddHUNT(0x6000,0x0400,K,V,M);
                    I = 0;
                    /* Show number of found locations */
                    sprintf(S,"Found %d locations.\n",J);
                    for(J=0;S[J];J++) if(S[J]=='\n') S[J]='\0';
                    CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Initial Search",S);
                    break;
                }
              }
              I=0;
              break;

            case 3: ScanHUNT();
            /* Fall through */
            case 4: /* Show current cheats */
              /* Find current number of locations, limit it to 32 */
              K = TotalHUNT();
              K = K<32? K:32;

              /* If no locations, show a warning */
              if(!K)
              {
                CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Empty","No cheats found.\0\0");
                I=0;
                break;
              }

              /* Show cheat selection dialog */
              for(I=1,M=0;I;)
              {
                /* Compose dialog */
                sprintf(S,"Found %d Cheats\n",K);
                for(J=0;(J<K)&&(strlen(S)<sizeof(S)-64);++J)
                {
                  if(!(PP=(char *)HUNT2Cheat(J,HUNT_COLECO))) break;
                  if(strlen(PP)>9) { PP[8]=CON_DOTS;PP[9]='\0'; }
                  sprintf(S+strlen(S),"%-9s %c\n",PP,M&(1<<J)? CON_CHECK:' ');
                }
                strcat(S,"  \nAdd cheats\n");
     
                /* Number of shown locations */
                K=J;
     
                /* Replace all EOLNs with zeroes */
                for(J=0;S[J];J++) if(S[J]=='\n') S[J]='\0';
     
                /* Run menu */
                I=CONMenu(-1,-1,-1,16,CLR_TEXT,CLR_BACK2,S,I);
     
                /* Toggle checkmarks */
                if((I>=1)&&(I<=K)) M^=1<<(I-1);
                else if(I)
                {
                  /* If there are cheats to add, drop out */
                  if(!M)
                  {
                    CONMsg(-1,-1,-1,-1,CLR_WHITE,CLR_INFO,"Empty","No cheats chosen.\0\0");
                    I=0;
                    break;
                  }

                  /* Disable and clear current cheats */
                  ResetCheats();

                  /* Add found cheats */
                  for(J=0;J<K;++J)
                    if((M&(1<<J))&&(P=(char *)HUNT2Cheat(J,HUNT_COLECO)))
                      for(T=0;P;P=T)
                      {
                        T=strchr(P,';');
                        if(T) *T++='\0';
                        AddCheat(P);
                      }

                  /* Activate new cheats */
                  Cheats(CHTS_ON);
                  I=0;
                }
              }

              /* Done with the menu */
              I=0;
              break;
          }
        }
        J=0;
        break;

      case 15: /* Reset */
        ResetColeco(Mode);
        J=0;
        break;
      case 16: /* Quit */
        ExitNow=1;
        J=0;
        break;
      case 18: /* Done */
        J=0;
        break;
    }
  }
}
