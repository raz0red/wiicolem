/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                       AdamNet.h                         **/
/**                                                         **/
/** This file contains declarations for the AdamNet I/O     **/
/** interface found in Coleco Adam home computer.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2019                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef ADAMNET_H
#define ADAMNET_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef unsigned short word;
#endif

/** ReadPCB() ************************************************/
/** Read value from a given PCB or DCB address.             **/
/*************************************************************/
void ReadPCB(word A);

/** WritePCB() ***********************************************/
/** Write value to a given PCB or DCB address.              **/
/*************************************************************/
void WritePCB(word A,byte V);

/** ResetPCB() ***********************************************/
/** Reset PCB and attached hardware.                        **/
/*************************************************************/
void ResetPCB(void);

#ifdef __cplusplus
}
#endif
#endif /* ADAMNET_H */
