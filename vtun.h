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

typedef enum {
	VTUN_MODE_CLIENT,
	VTUN_MODE_SERVER,
	VTUN_MODE_UNSPECIFIED
} vtun_mode_t;

typedef struct _vtun_info vtun_info_t;

struct _vtun_info {
	union {
		uint8_t buf[65536];
		struct ip iphdr;
	};

	struct sockaddr_in addr;
	int local;
	vtun_mode_t mode;
#ifdef DEBUG
	char name1[32], name2[32];
#endif
	int peer;
	DES_key_schedule sched[3];
	DES_cblock temp;
	void (*xfer_l2p)(vtun_info_t *info);
	void (*xfer_p2l)(vtun_info_t *info);
};

extern void vtun_3des_decode(vtun_info_t *info, ssize_t *len);
extern void vtun_3des_encode(vtun_info_t *info, ssize_t *len);
extern void vtun_dump_iphdr(vtun_info_t *info);

#endif /* __include_vtun_h__ */
