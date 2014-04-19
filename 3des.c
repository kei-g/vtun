#include "3des.h"

static void vtun_3des(info, enc)
	vtun_info_t *info;
	int enc;
{
	ssize_t i;
	DES_cblock temp;

	for (i = 0; i < info->buflen; i += sizeof(temp)) {
		memset(&temp, 0, sizeof(temp));
		DES_ecb3_encrypt((DES_cblock *)&info->buf[i], &temp,
			&info->sched[0], &info->sched[1], &info->sched[2], enc);
		memcpy(&info->buf[i], &temp, sizeof(temp));
	}
}

void vtun_3des_decode(info)
	vtun_info_t *info;
{
	vtun_3des(info, DES_DECRYPT);
	info->buflen = ntohs(info->iphdr.ip_len);
}

void vtun_3des_encode(info)
	vtun_info_t *info;
{
	info->buflen = (info->buflen + 7) & ~7;
	vtun_3des(info, DES_ENCRYPT);
}
