#include "codec.h"

static void generate_random(uint8_t *buf, size_t bufsize);

void vtun_decode(info)
	vtun_info_t *info;
{
	int buflen, len;
	buflen = ntohs(info->obj.iphdr->ip_len);
	len = 0;
	EVP_DecryptUpdate(info->dec, info->tmp, &len, info->obj.buf, buflen);
	info->buflen = len;
}

void vtun_encode(info)
	vtun_info_t *info;
{
	int len;
	len = 0;
	EVP_EncryptUpdate(info->enc, info->obj.buf, &len,
		info->tmp, info->buflen);
	info->buflen = len;
}

void vtun_generate_iv(iv)
	uint8_t iv[12];
{
	generate_random(iv, 12);
}

void vtun_generate_key(key)
	uint8_t key[32];
{
	generate_random(key, 32);
}

static void generate_random(buf, bufsize)
	uint8_t *buf;
	size_t bufsize;
{
	int fd;
	ssize_t len;
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		perror("open(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
	len = read(fd, buf, bufsize);
	if (len < 0) {
		perror("read(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
	if (close(fd) < 0) {
		perror("close(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
}
