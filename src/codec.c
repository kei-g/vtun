#include "codec.h"

#include <fcntl.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void generate_random(uint8_t *buf, size_t bufsize);

void vtun_decode(vtun_info_t *info)
{
	int len = 0;
	EVP_DecryptUpdate(info->dec, info->tmp, &len,
		info->obj.buf, info->buflen);
	info->buflen = len;
}

void vtun_encode(vtun_info_t *info)
{
	int len = 0;
	EVP_EncryptUpdate(info->enc, info->obj.buf, &len,
		(void *)&info->tun.iphdr, info->buflen);
	info->buflen = len;
}

void vtun_generate_iv(uint8_t iv[12])
{
	generate_random(iv, 12);
}

void vtun_generate_key(uint8_t key[32])
{
	generate_random(key, 32);
}

static void generate_random(uint8_t *buf, size_t bufsize)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		perror("open(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
	ssize_t len = read(fd, buf, bufsize);
	if (len < 0) {
		perror("read(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
	if (close(fd) < 0) {
		perror("close(\"/dev/urandom\")");
		exit(EXIT_FAILURE);
	}
}
