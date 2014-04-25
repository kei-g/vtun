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

char *vtun_3des_string_of_key(b, key)
	base64_t b;
	DES_cblock key[3];
{
	ssize_t i;
	uint8_t buf[21], *p, *k;

	for (i = 0, p = buf; i < 3; i++) {
		k = (uint8_t *)&key[i];
		*p++ = ((k[0]     ) & 0xfe) | ((k[1] >> 7) & 0x01);
		*p++ = ((k[1] << 1) & 0xfc) | ((k[2] >> 6) & 0x03);
		*p++ = ((k[2] << 2) & 0xf8) | ((k[3] >> 5) & 0x07);
		*p++ = ((k[3] << 3) & 0xf0) | ((k[4] >> 4) & 0x0f);
		*p++ = ((k[4] << 4) & 0xe0) | ((k[5] >> 3) & 0x1f);
		*p++ = ((k[5] << 5) & 0xc0) | ((k[6] >> 2) & 0x3f);
		*p++ = ((k[6] << 6) & 0x80) | ((k[7] >> 1) & 0x7f);
	}
	return base64_encode(b, buf, sizeof(buf));
}

void vtun_3des_decode_key(b, key, msg)
	base64_t b;
	DES_cblock key[3];
	const char *msg;
{
	uint8_t k[21], *p, *d;
	ssize_t i;

	memset(k, 0, sizeof(k));
	base64_decode(b, k, msg);

	for (i = 0, p = k, d = (uint8_t *)key; i < 3; i++, p += 7) {
		*d++ = (p[0] & 0xfe);
		*d++ = ((p[0] & 0x01) << 7) | ((p[1] & 0xfc) >> 1);
		*d++ = ((p[1] & 0x03) << 6) | ((p[2] & 0xf8) >> 2);
		*d++ = ((p[2] & 0x07) << 5) | ((p[3] & 0xf0) >> 3);
		*d++ = ((p[3] & 0x0f) << 4) | ((p[4] & 0xe0) >> 4);
		*d++ = ((p[4] & 0x1f) << 3) | ((p[5] & 0xc0) >> 5);
		*d++ = ((p[5] & 0x3f) << 2) | ((p[6] & 0x80) >> 6);
		*d++ = ((p[6] & 0x7f) << 1);
		DES_set_odd_parity(&key[i]);
		if (DES_is_weak_key(&key[i])) {
			(void)fprintf(stderr, "Specified key is too weak.\n");
			exit(1);
		}
	}
}
