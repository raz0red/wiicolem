/*
  Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com
*/

#include "md5.h"
#include <string.h>

#define T_MASK ((md5_word_t)~0)
#define T1 (T_MASK ^ 0x28955B88)
#define T2 (T_MASK ^ 0x173848AA)
#define T3  0x242070DB
#define T4 (T_MASK ^ 0x3E423112)
#define T5 (T_MASK ^ 0x0A83F051)
#define T6  0x4787C62A
#define T7 (T_MASK ^ 0x57CFB9ED)
#define T8 (T_MASK ^ 0x02B96AFF)
#define T9  0x698098D8
#define T10 (T_MASK ^ 0x74BB0851)
#define T11 (T_MASK ^ 0x0000A44F)
#define T12 (T_MASK ^ 0x76A32842)
#define T13  0x6B901122
#define T14 (T_MASK ^ 0x02678E6D)
#define T15 (T_MASK ^ 0x5986BC72)
#define T16  0x49B40821
#define T17 (T_MASK ^ 0x09E1DA9E)
#define T18 (T_MASK ^ 0x3FBF4CC0)
#define T19  0x265E5A51
#define T20 (T_MASK ^ 0x16493856)
#define T21 (T_MASK ^ 0x29D0EFA3)
#define T22  0x02441453
#define T23 (T_MASK ^ 0x275E197F)
#define T24 (T_MASK ^ 0x182C0438)
#define T25  0x21E1CDE6
#define T26 (T_MASK ^ 0x3CC8F82A)
#define T27 (T_MASK ^ 0x0B2AF279)
#define T28  0x455A14ED
#define T29 (T_MASK ^ 0x561C16FB)
#define T30 (T_MASK ^ 0x03105C08)
#define T31  0x676F02D9
#define T32 (T_MASK ^ 0x72D5B376)
#define T33 (T_MASK ^ 0x0005C6BE)
#define T34 (T_MASK ^ 0x788E097F)
#define T35  0x6D9D6122
#define T36 (T_MASK ^ 0x021AC7F4)
#define T37 (T_MASK ^ 0x5B4115BC)
#define T38  0x4BDECFA9
#define T39 (T_MASK ^ 0x0944B4A0)
#define T40 (T_MASK ^ 0x41404390)
#define T41  0x289B7EC6
#define T42 (T_MASK ^ 0x155ED806)
#define T43 (T_MASK ^ 0x2B10CF7B)
#define T44  0x04881D05
#define T45 (T_MASK ^ 0x262B2FC7)
#define T46 (T_MASK ^ 0x1924661B)
#define T47  0x1FA27CF8
#define T48 (T_MASK ^ 0x3B53A99B)
#define T49 (T_MASK ^ 0x0BD6DDBC)
#define T50  0x432AFF97
#define T51 (T_MASK ^ 0x546BDC59)
#define T52 (T_MASK ^ 0x036C5FC7)
#define T53  0x655B59C3
#define T54 (T_MASK ^ 0x70F3336E)
#define T55 (T_MASK ^ 0x00100B83)
#define T56 (T_MASK ^ 0x7A7BA22F)
#define T57  0x6FA87E4F
#define T58 (T_MASK ^ 0x01D31920)
#define T59 (T_MASK ^ 0x5CFEBCEC)
#define T60  0x4E0811A1
#define T61 (T_MASK ^ 0x08AC817E)
#define T62 (T_MASK ^ 0x42C50DCB)
#define T63  0x2AD7D2BB
#define T64 (T_MASK ^ 0x14792C6F)

static void md5_process(md5_state_t *pms, const md5_byte_t *data)
{
    md5_word_t a = pms->abcd[0], b = pms->abcd[1],
               c = pms->abcd[2], d = pms->abcd[3];
    md5_word_t t;
    md5_word_t xbuf[16];
    const md5_word_t *X;

    {
        static const int w = 1;
        if (*((const md5_byte_t *)&w))
        {
            /* little-endian, use as-is */
            X = (const md5_word_t *)data;
        }
        else
        {
            /* big-endian, swap */
            int i;
            for (i = 0; i < 16; ++i, data += 4)
                xbuf[i] = (md5_word_t)data[0] +
                           ((md5_word_t)data[1] << 8) +
                           ((md5_word_t)data[2] << 16) +
                           ((md5_word_t)data[3] << 24);
            X = xbuf;
        }
    }

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti) \
  t = a + F(b,c,d) + X[k] + Ti; \
  a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  0,  7,  T1);
    SET(d, a, b, c,  1, 12,  T2);
    SET(c, d, a, b,  2, 17,  T3);
    SET(b, c, d, a,  3, 22,  T4);
    SET(a, b, c, d,  4,  7,  T5);
    SET(d, a, b, c,  5, 12,  T6);
    SET(c, d, a, b,  6, 17,  T7);
    SET(b, c, d, a,  7, 22,  T8);
    SET(a, b, c, d,  8,  7,  T9);
    SET(d, a, b, c,  9, 12, T10);
    SET(c, d, a, b, 10, 17, T11);
    SET(b, c, d, a, 11, 22, T12);
    SET(a, b, c, d, 12,  7, T13);
    SET(d, a, b, c, 13, 12, T14);
    SET(c, d, a, b, 14, 17, T15);
    SET(b, c, d, a, 15, 22, T16);
#undef SET
#undef F

#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti) \
  t = a + G(b,c,d) + X[k] + Ti; \
  a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  1,  5, T17);
    SET(d, a, b, c,  6,  9, T18);
    SET(c, d, a, b, 11, 14, T19);
    SET(b, c, d, a,  0, 20, T20);
    SET(a, b, c, d,  5,  5, T21);
    SET(d, a, b, c, 10,  9, T22);
    SET(c, d, a, b, 15, 14, T23);
    SET(b, c, d, a,  4, 20, T24);
    SET(a, b, c, d,  9,  5, T25);
    SET(d, a, b, c, 14,  9, T26);
    SET(c, d, a, b,  3, 14, T27);
    SET(b, c, d, a,  8, 20, T28);
    SET(a, b, c, d, 13,  5, T29);
    SET(d, a, b, c,  2,  9, T30);
    SET(c, d, a, b,  7, 14, T31);
    SET(b, c, d, a, 12, 20, T32);
#undef SET
#undef G

#define H(x, y, z) ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti) \
  t = a + H(b,c,d) + X[k] + Ti; \
  a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  5,  4, T33);
    SET(d, a, b, c,  8, 11, T34);
    SET(c, d, a, b, 11, 16, T35);
    SET(b, c, d, a, 14, 23, T36);
    SET(a, b, c, d,  1,  4, T37);
    SET(d, a, b, c,  4, 11, T38);
    SET(c, d, a, b,  7, 16, T39);
    SET(b, c, d, a, 10, 23, T40);
    SET(a, b, c, d, 13,  4, T41);
    SET(d, a, b, c,  0, 11, T42);
    SET(c, d, a, b,  3, 16, T43);
    SET(b, c, d, a,  6, 23, T44);
    SET(a, b, c, d,  9,  4, T45);
    SET(d, a, b, c, 12, 11, T46);
    SET(c, d, a, b, 15, 16, T47);
    SET(b, c, d, a,  2, 23, T48);
#undef SET
#undef H

#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti) \
  t = a + I(b,c,d) + X[k] + Ti; \
  a = ROTATE_LEFT(t, s) + b
    SET(a, b, c, d,  0,  6, T49);
    SET(d, a, b, c,  7, 10, T50);
    SET(c, d, a, b, 14, 15, T51);
    SET(b, c, d, a,  5, 21, T52);
    SET(a, b, c, d, 12,  6, T53);
    SET(d, a, b, c,  3, 10, T54);
    SET(c, d, a, b, 10, 15, T55);
    SET(b, c, d, a,  1, 21, T56);
    SET(a, b, c, d,  8,  6, T57);
    SET(d, a, b, c, 15, 10, T58);
    SET(c, d, a, b,  6, 15, T59);
    SET(b, c, d, a, 13, 21, T60);
    SET(a, b, c, d,  4,  6, T61);
    SET(d, a, b, c, 11, 10, T62);
    SET(c, d, a, b,  2, 15, T63);
    SET(b, c, d, a,  9, 21, T64);
#undef SET
#undef I

    pms->abcd[0] += a;
    pms->abcd[1] += b;
    pms->abcd[2] += c;
    pms->abcd[3] += d;
}

void md5_init(md5_state_t *pms)
{
    pms->count[0] = pms->count[1] = 0;
    pms->abcd[0] = 0x67452301;
    pms->abcd[1] = 0xEFCDAB89;
    pms->abcd[2] = 0x98BADCFE;
    pms->abcd[3] = 0x10325476;
}

void md5_append(md5_state_t *pms, const md5_byte_t *data, size_t nbytes)
{
    const md5_byte_t *p = data;
    size_t left = nbytes;
    int offset = (pms->count[0] >> 3) & 63;
    md5_word_t nbits = (md5_word_t)(nbytes << 3);

    if (nbytes <= 0) return;

    pms->count[1] += (md5_word_t)(nbytes >> 29);
    pms->count[0] += nbits;
    if (pms->count[0] < nbits)
        pms->count[1]++;

    if (offset) {
        int copy = (offset + nbytes > 64 ? 64 - offset : (int)nbytes);
        memcpy(pms->buf + offset, p, copy);
        if (offset + copy < 64) return;
        p += copy;
        left -= copy;
        md5_process(pms, pms->buf);
    }

    for (; left >= 64; p += 64, left -= 64)
        md5_process(pms, p);

    if (left)
        memcpy(pms->buf, p, left);
}

void md5_finish(md5_state_t *pms, md5_byte_t digest[16])
{
    static const md5_byte_t pad[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    md5_byte_t data[8];
    int i;

    for (i = 0; i < 8; ++i)
        data[i] = (md5_byte_t)(pms->count[i >> 2] >> ((i & 3) << 3));

    md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
    md5_append(pms, data, 8);

    for (i = 0; i < 16; ++i)
        digest[i] = (md5_byte_t)(pms->abcd[i >> 2] >> ((i & 3) << 3));
}
