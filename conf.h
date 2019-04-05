#ifndef __include_conf_h__
#define __include_conf_h__

#include "vtun.h"

typedef enum {
	VTUN_MODE_CLIENT,
	VTUN_MODE_SERVER,
	VTUN_MODE_UNSPECIFIED,
} vtun_mode_t;

typedef struct _vtun_conf vtun_conf_t;
typedef struct _vtun_route vtun_route_t;

struct _vtun_conf {
	struct sockaddr_in addr;
	int dev, sock;
	char dev_type[16], ifa_src[16], ifa_dst[16], ifr_name[16];
	uint32_t ifa_mask;
	vtun_mode_t mode;
	vtun_route_t *routes;
	DES_key_schedule sched[3];
	int (*xfer_p2l)(vtun_info_t *info, const struct sockaddr_in *addr);
};

struct _vtun_route {
	char dst[32];
	vtun_route_t *next;
};

extern void vtun_conf_init(vtun_conf_t *conf, const char *path);

#endif /* __include_conf_h__ */
