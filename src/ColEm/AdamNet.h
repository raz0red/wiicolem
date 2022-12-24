/** ColEm: portable Coleco emulator **************************/
/**                                                         **/
/**                       AdamNet.h                         **/
/**                                                         **/
/** This file contains declarations for the AdamNet I/O     **/
/** interface found in Coleco Adam home computer.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2021                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef ADAMNET_H
#define ADAMNET_H
#ifdef __cplusplus
extern "C" {
#endif

/** Adam Key Codes *******************************************/
#define KEY_CONTROL    CON_CONTROL
#define KEY_SHIFT      CON_SHIFT
#define KEY_CAPS       CON_CAPS
#define KEY_ESC        27
#define KEY_BS         8   // + SHIFT = 184
#define KEY_TAB        9   // + SHIFT = 185
#define KEY_ENTER      13 
#define KEY_QUOTE      '\''
#define KEY_BQUOTE     '`'
#define KEY_BSLASH     '\\'
#define KEY_COMMA      ','
#define KEY_DOT        '.'
#define KEY_SLASH      '/'
#define KEY_ASTERISK   '*'
#define KEY_HOME       128
#define KEY_F1         129 // + SHIFT = 137 
#define KEY_F2         130 // + SHIFT = 138
#define KEY_F3         131 // + SHIFT = 139
#define KEY_F4         132 // + SHIFT = 140
#define KEY_F5         133 // + SHIFT = 141
#define KEY_F6         134 // + SHIFT = 142
#define KEY_WILDCARD   144 // + SHIFT = 152
#define KEY_UNDO       145 // + SHIFT = 153
#define KEY_MOVE       146 // + SHIFT = 154 (COPY)
#define KEY_STORE      147 // + SHIFT = 155 (FETCH)
#define KEY_INS        148 // + SHIFT = 156
#define KEY_PRINT      149 // + SHIFT = 157
#define KEY_CLEAR      150 // + SHIFT = 158
#define KEY_DEL        151 // + SHIFT = 159, + CTRL = 127
#define KEY_UP         160 // + CTRL = 164, + HOME = 172
#define KEY_RIGHT      161 // + CTRL = 165, + HOME = 173
#define KEY_DOWN       162 // + CTRL = 166, + HOME = 174
#define KEY_LEFT       163 // + CTRL = 167, + HOME = 175
#define KEY_DIAG_NE    168
#define KEY_DIAG_SE    169
#define KEY_DIAG_SW    170
#define KEY_DIAG_NW    171

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

/** PutKBD() *************************************************/
/** Add a new key to the keyboard buffer.                   **/
/*************************************************************/
void PutKBD(unsigned int Key);

#ifdef __cplusplus
}
#endif
#endif /* ADAMNET_H */
