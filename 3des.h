#ifndef __include_3des_h__
#define __include_3des_h__

#include "vtun.h"
#include "base64.h"

void vtun_3des_decode(vtun_info_t *info);
void vtun_3des_encode(vtun_info_t *info);

char *vtun_3des_string_of_key(base64_t b, DES_cblock key[3]);
void vtun_3des_decode_key(base64_t b, DES_cblock key[3], const char *msg);
void vtun_3des_generate_key(DES_cblock key[3]);

#endif /* __include_3des_h__ */
