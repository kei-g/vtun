#ifndef __include_codec_h__
#define __include_codec_h__

#include "vtun.h"

void vtun_decode(vtun_info_t *info);
void vtun_encode(vtun_info_t *info);

void vtun_generate_iv(uint8_t iv[12]);
void vtun_generate_key(uint8_t key[32]);

#endif /* __include_codec_h__ */
