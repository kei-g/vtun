#ifndef __include_base64_h__
#define __include_base64_h__

#include <sys/types.h>

typedef struct _base64 *base64_t;

base64_t base64_alloc(void);
void base64_free(base64_t *pb);

ssize_t base64_decode(base64_t b, void *buf, const char *msg);
char *base64_encode(base64_t b, const void *buf, size_t len);

#endif /* __include_base64_h__ */
