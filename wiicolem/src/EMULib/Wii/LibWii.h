/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibWii.h                         **/
/**                                                         **/
/** This file contains Wii-dependent definitions and        **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/*
WiiColem : Port of the Colem ColecoVision Emulator for the Wii

Copyright (C) 2010
raz0red (www.twitchasylum.com)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.
*/

#ifndef LIBWII_H
#define LIBWII_H

#ifdef __cplusplus
extern "C" {
#endif

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent). The     **/
/** Delay value is given in millseconds and determines how  **/
/** much buffering will be done.                            **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Delay);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

#ifdef __cplusplus
}
#endif
#endif /* LIBWII_H */
