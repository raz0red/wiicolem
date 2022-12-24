#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Coleco.h"
#include "Sound.h"

#include "wii_types.h"
#include <emscripten.h>

#define COLECO_WIDTH 272
#define COLECO_HEIGHT 200

// Sound information
static int UseSound = 48000; /* Audio sampling frequency (Hz)  */
static int SndSwitch;        /* Mask of enabled sound channels */
static int SndVolume;        /* Master volume for audio        */

extern int MasterSwitch;
extern int *AudioBuf;

extern "C" int EmStart() {
    /* Initialize video */
    ScrWidth = COLECO_WIDTH;
    ScrHeight = COLECO_HEIGHT;
    ScrBuffer = NULL;

    // Initialize and run ColEm
    InitSound(UseSound, 150);
    SndSwitch = (1 << (SN76489_CHANNELS + AY8910_CHANNELS)) - 1;
    SndVolume = 255 / SN76489_CHANNELS;
    SetChannels(SndVolume, SndSwitch);

    StartColeco(NULL);

    // Load the ROM
    int succeeded = LoadROM("/rom.col");
    if (!succeeded) {
        printf("Error loading rom\n");
        return 0;
    }

    // Set the volume
    int Volume = 7;
    SetChannels((3 + (Volume * 4)), MasterSwitch);
    ResetColeco(Mode);

    return 1;
}

extern "C" void EmStep() {
    RunZ80(&CPU);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void) {
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void) {
    return (1);
}

/** Mouse() **************************************************/
/** This function is called to poll mouse. It should return **/
/** the X coordinate (-512..+512) in the lower 16 bits, the **/
/** Y coordinate (-512..+512) in the next 14 bits, and the  **/
/** mouse buttons in the upper 2 bits. All values should be **/
/** counted from the screen center!                         **/
/*************************************************************/
unsigned int Mouse(void) {
    return 0;
}

/** Joystick() ***********************************************/
/** This function is called to poll joysticks. It should    **/
/** return 2x16bit values in a following way:               **/
/**                                                         **/
/**      x.FIRE-B.x.x.L.D.R.U.x.FIRE-A.x.x.N3.N2.N1.N0      **/
/**                                                         **/
/** Least significant bit on the right. Active value is 1.  **/
/*************************************************************/

unsigned int Joystick(void) {
    int samples = RenderAndPlayAudio((UseSound/((Mode & CV_PAL) ? 50 : 60)));
    return 0x000200002;
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
int SetColor(byte N, byte R, byte G, byte B) {
    return (((int)R<<16)|((int)G<<8)|B);
}

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
void RefreshScreen(void* Buffer, int Width, int Height) {
    EM_ASM({
        window.emulator.drawScreen($0, $1, $2);
    }, Buffer, Width, Height);
}
