#ifndef __include_vtun_h__
#define __include_vtun_h__

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <openssl/des.h>

typedef struct _vtun_info vtun_info_t;

struct _vtun_info {
	union {
		uint8_t buf[65536];
		DES_cblock cb[1];
		struct ip iphdr[1];
	};
	ssize_t buflen;

	struct sockaddr_in addr;
	int dev, ignore, sock;
	struct timespec *keepalive;
#ifdef DEBUG
	char name1[32], name2[32];
#endif
	DES_key_schedule sched[3];
	int (*xfer_p2l)(vtun_info_t *info, const struct sockaddr_in *addr);
};

#endif /* __include_vtun_h__ */
