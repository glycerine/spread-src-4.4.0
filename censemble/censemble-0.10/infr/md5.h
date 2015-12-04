/* MD5 message digest */

#ifndef _md5
#define _md5

#include "infr/config.h"
#include "infr/trans.h"

typedef struct MD5Context {
        uint32_t buf[4];
        uint32_t bits[2];
        unsigned char in[64];
} MD5Context ;

void MD5Init (struct MD5Context *context);
void MD5Update (struct MD5Context *context, const unsigned char *buf, unsigned len);
void MD5Final (unsigned char *digest, struct MD5Context *ctx);
void MD5Transform (uint32_t *buf, uint32_t *in);

#endif
