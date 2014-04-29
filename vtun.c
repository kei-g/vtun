#include "vtun.h"

#include "conf.h"
#include "xfer.h"

static void vtun_info_init(info, conf, w)
	vtun_info_t *info;
	const vtun_conf_t *conf;
	struct timespec *w;
{
	info->addr = conf->addr;
	info->dev = conf->dev;
	info->ignore = conf->mode == VTUN_MODE_SERVER;
	info->sock = conf->sock;
	info->keepalive = conf->mode == VTUN_MODE_CLIENT ? w : NULL;
	memcpy(info->sched, conf->sched, sizeof(info->sched));
	info->xfer_p2l = conf->xfer_p2l;

	if (conf->mode == VTUN_MODE_CLIENT)
		vtun_xfer_keepalive(info);
}

static void vtun_main(info)
	vtun_info_t *info;
{
	int kq, n = 0;
	struct kevent kev[2];
	void (*func)(vtun_info_t *info);

	if ((kq = kqueue()) < 0) {
		perror("kqueue");
		exit(1);
	}

	EV_SET(&kev[n++], info->dev, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&kev[n++], info->sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
	for (;;) {
		if ((n = kevent(kq, kev, n, kev, 1, info->keepalive)) < 0) {
			perror("kevent");
			exit(1);
		}
		if (n == 0)
			func = vtun_xfer_keepalive;
		else
			func = kev->ident == info->dev ? vtun_xfer_l2p : vtun_xfer_p2l;
		(*func)(info);
		n = 0;
	}
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	vtun_conf_t conf;
	vtun_info_t *info;
	struct timespec w;

	info = (vtun_info_t *)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	vtun_conf_init(&conf, argc < 2 ? "/usr/local/etc/vtun.conf" : argv[1]);

	w.tv_sec = 30;
	w.tv_nsec = 0;
	vtun_info_init(info, &conf, &w);

	vtun_main(info);

	return (0);
}
