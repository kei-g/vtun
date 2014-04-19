#ifndef __include_conf_h__
#define __include_conf_h__

#include "vtun.h"

typedef enum {
	VTUN_MODE_CLIENT,
	VTUN_MODE_SERVER,
	VTUN_MODE_UNSPECIFIED,
} vtun_mode_t;

typedef struct _vtun_conf vtun_conf_t;

struct _vtun_conf {
	struct sockaddr_in addr;
	int dev, sock;
	vtun_mode_t mode;
	DES_key_schedule sched[3];
	int (*xfer_p2l)(vtun_info_t *info, const struct sockaddr_in *addr);
};

extern void vtun_conf_init(vtun_conf_t *conf, const char *path);

#endif /* __include_conf_h__ */
