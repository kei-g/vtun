#include "base64.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct _base64 {
	uint8_t tbl[256];
	char w[65];
};

base64_t base64_alloc(void)
{
	base64_t b;
	ssize_t i;
	char c;

	b = (base64_t)malloc(sizeof(*b));
	if (!b) {
		perror("malloc");
		exit(1);
	}

	for (i = 0, c = 'A'; c <= 'Z'; c++)
		b->w[i++] = c;
	for (c = 'a'; c <= 'z'; c++)
		b->w[i++] = c;
	for (c = '0'; c <= '9'; c++)
		b->w[i++] = c;
	b->w[i++] = '+';
	b->w[i++] = '/';
	b->w[i++] = '=';

	for (i = 0; i < sizeof(b->w); i++)
		b->tbl[(int)b->w[i]] = (uint8_t)(i & 63);

	return (b);
}

void base64_free(pb)
	base64_t *pb;
{
	base64_t b;

	b = *pb;
	*pb = NULL;

	if (b)
		free(b);
}

ssize_t base64_decode(b, buf, msg)
	base64_t b;
	void *buf;
	const char *msg;
{
	const char *p = msg;
	ssize_t i, j, k = 0;
	uint8_t *const dst = buf, c[4];

	while (*p) {
		for (i = 0, j = 3; i < 4; i++) {
			c[i] = b->tbl[(int)*p++];
			if (*p == '=')
				j--;
		}
		for (i = 1; i <= j; i++)
			dst[k++] = c[i - 1] << (i * 2) | c[i] >> ((3 - i) * 2);
	}
	return (k);
}

char *base64_encode(b, buf, len)
	base64_t b;
	const void *buf;
	size_t len;
{
	char *msg;
	size_t i, j = 0, k = 0;
	uint32_t x = 0;

	msg = (char *)malloc(((len * 4 / 3 + 3) & ~3) + 1);
	if (!msg) {
		perror("malloc");
		exit(1);
	}

	for (i = 0; i < len; i++) {
		x = x << 8 | ((uint8_t *)buf)[i];
		for (j += 8; 6 <= j; j -= 6)
			msg[k++] = b->w[(x >> (j - 6)) & 63];
	}
	if (0 < j) {
		x <<= 6 - j;
		msg[k++] = b->w[x & 63];
	}
	while (k & 3)
		msg[k++] = '=';
	msg[k++] = '\0';
	return (msg);
}
