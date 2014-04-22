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
}

static void vtun_keepalive(info)
	vtun_info_t *info;
{
	uint8_t buf[4];
	ssize_t len;

	memset(buf, 0, sizeof(buf));
	len = sendto(info->sock, buf, sizeof(buf), 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (len < 0) {
		perror("sendto");
		exit(1);
	}
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	vtun_conf_t conf;
	vtun_info_t *info;
	struct timespec w;
	int kq, n;
	struct kevent kev[2];
	void (*func)(vtun_info_t *info);

	info = (vtun_info_t *)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	if ((kq = kqueue()) < 0) {
		perror("kqueue");
		exit(1);
	}

	vtun_conf_init(&conf, "vtun.conf");

	w.tv_sec = 30;
	w.tv_nsec = 0;
	vtun_info_init(info, &conf, &w);

	EV_SET(&kev[0], info->dev, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&kev[1], info->sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(kq, kev, sizeof(kev) / sizeof(kev[0]), NULL, 0, NULL) < 0) {
		perror("kevent");
		exit(1);
	}

	for (;;) {
		if ((n = kevent(kq, NULL, 0, kev, 1, info->keepalive)) < 0) {
			perror("kevent");
			exit(1);
		}
		if (n == 0)
			vtun_keepalive(info);
		else {
			func = kev->ident == info->dev ? vtun_xfer_l2p : vtun_xfer_p2l;
			memset(info->buf, 0, sizeof(info->buf));
			(*func)(info);
		}
	}

	return (0);
}
