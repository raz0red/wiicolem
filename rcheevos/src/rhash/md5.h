#ifndef RC_MD5_H
#define RC_MD5_H

#include <stddef.h>

typedef unsigned char md5_byte_t;
typedef unsigned int  md5_word_t;

typedef struct md5_state_s {
    md5_word_t count[2];
    md5_word_t abcd[4];
    md5_byte_t buf[64];
} md5_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void md5_init(md5_state_t *pms);
void md5_append(md5_state_t *pms, const md5_byte_t *data, size_t nbytes);
void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);

#ifdef __cplusplus
}
#endif

#endif /* RC_MD5_H */
