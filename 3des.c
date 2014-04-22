#include "3des.h"

static void vtun_3des(info, enc)
	vtun_info_t *info;
	int enc;
{
	DES_key_schedule *const sched = info->sched;
	ssize_t i;
	DES_cblock *cb;

	for (i = 0, cb = info->cb; i < info->buflen; i += sizeof(*cb), cb++)
		DES_ecb3_encrypt(cb, cb, &sched[0], &sched[1], &sched[2], enc);
}

void vtun_3des_decode(info)
	vtun_info_t *info;
{
	vtun_3des(info, DES_DECRYPT);
	info->buflen = ntohs(info->iphdr->ip_len);
}

void vtun_3des_encode(info)
	vtun_info_t *info;
{
	info->buflen = (info->buflen + 7) & ~7;
	vtun_3des(info, DES_ENCRYPT);
}
