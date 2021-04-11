#ifndef __include_vtun_h__
#define __include_vtun_h__

#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <time.h>

typedef struct _vtun_info vtun_info_t;

struct _vtun_info {
	union {
		uint8_t all[65536];
		struct {
			uint8_t cmd;
			union {
				uint8_t buf[65535];
#if defined(__FreeBSD__)
				struct ip iphdr[1];
#elif defined(__linux__)
				struct iphdr iphdr[1];
#endif
			};
		} obj;
	};
	ssize_t buflen;

	struct sockaddr_in addr;
	EVP_CIPHER_CTX *dec, *enc;
	int dev, ignore, sock;
	struct timespec *keepalive;
#ifdef DEBUG
	char name1[32], name2[32];
#endif
	uint8_t tmp[65536];
	int (*xfer_p2l)(vtun_info_t *info, const struct sockaddr_in *addr);
};

#endif /* __include_vtun_h__ */
