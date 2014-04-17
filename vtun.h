#ifndef __include_vtun_h__
#define __include_vtun_h__

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct {
	union {
		uint8_t buf[65536];
		struct ip iphdr;
	};
	int local;
	vtun_mode_t mode;
#ifdef DEBUG
	char name1[32], name2[32];
#endif
	int peer;
	struct sockaddr_in server;
	DES_key_schedule sched[3];
	DES_cblock temp;
} *vtun_info_t;

extern vtun_info_t vtun_init(const char *path);
extern void vtun_dump_iphdr(vtun_info_t info);

#endif /* __include_vtun_h__ */
