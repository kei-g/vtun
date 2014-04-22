#ifndef __include_base64_h__
#define __include_base64_h__

#include <sys/types.h>

typedef struct _vtun_base64 *vtun_base64_t;

vtun_base64_t vtun_base64_alloc(void);
void vtun_base64_free(vtun_base64_t *pb);

ssize_t vtun_base64_decode(vtun_base64_t b, void *buf, const char *msg);
char *vtun_base64_encode(vtun_base64_t b, const void *buf, size_t len);

#endif /* __include_base64_h__ */
