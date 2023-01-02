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

static unsigned int Input = 0;
static int AxisX1 = 0;
static int AxisY1 = 0;
static int AxisX2 = 0;
static int AxisY2 = 0;
static unsigned long EmOpts = 0;

extern "C" int EmStart() {
    /* Initialize video */
    ScrWidth = COLECO_WIDTH;
    ScrHeight = COLECO_HEIGHT;
    ScrBuffer = NULL;

    // 100% of frames
    UPeriod = 100;

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
    ResetColeco(Mode | EmOpts);

    return 1;
}

extern "C" void EmSetOpts(unsigned long opts) {
    EmOpts = opts;
}

extern "C" void EmStep() {
    RunZ80(&CPU);
}

extern "C" void EmSetInput(unsigned int input, int axisX1, int axisY1, int axisX2, int axisY2) {
    Input = input;
    AxisX1 = axisX1;
    AxisY1 = axisY1;
    AxisX2 = axisX2;
    AxisY2 = axisY2;
}

extern "C" int EmSaveState() {
    return SaveSTA("/freeze.sta");
}

extern "C" int EmLoadState() {
    return LoadSTA("/freeze.sta");
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

    int x = 0;
    int y = 0;

    x = AxisX1;
    y = AxisY1;

    //printf("%d, %d\n", AxisX1, AxisY1);

    x &= 0xFFFF;
    y &= 0x3FFF;

    return ((y << 16) | x);
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
    return Input;
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
