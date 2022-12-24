#include "EMULib.h"
#include "LibWii.h"
#include "Sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate, unsigned int Latency) {
    return (Rate);
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void) {
}

/** PauseAudio() *********************************************/
/** Pause/resume audio playback.                            **/
/*************************************************************/
int PauseAudio(int Switch) {
    return 0;
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void) {
    return 0;
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample* Data, unsigned int Length) {
    EM_ASM({
        window.emulator.audioCallback($0, $1);
    }, Data, Length);

    return Length;
}

/** ResetAudio() *********************************************/
/** Resets the audio buffers.                               **/
/*************************************************************/
void ResetAudio() {
}