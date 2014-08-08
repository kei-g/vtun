#include "vtun.h"

#include <libgen.h>
#include <sys/param.h>

#include "3des.h"
#include "base64.h"
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

static int vtun_main(info)
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

	return (0);
}

static int vtun_bn_generate_key(void)
{
	DES_cblock key[3];
	base64_t b;
	char *msg;

	vtun_3des_generate_key(key);

	b = base64_alloc();
	msg = vtun_3des_string_of_key(b, key);
	base64_free(&b);

	printf("%s\n", msg);
	free(msg);

	return (0);
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	char *bn, *confpath = NULL, *pidpath = NULL;
	int opt;
	vtun_conf_t conf;
	vtun_info_t *info;
	struct timespec w;

	bn = basename(*argv);
	if (strcmp(bn, "vtun-keygen") == 0)
		return vtun_bn_generate_key();

	while ((opt = getopt(argc, argv, "c:p:")) != -1)
		switch (opt) {
		case 'c':
			confpath = strdup(optarg);
			break;
		case 'p':
			pidpath = strdup(optarg);
			break;
		}

	if (!confpath)
		confpath = strdup("/usr/local/etc/vtun/vtun.conf");
	vtun_conf_init(&conf, confpath);
	free(confpath);

	if (pidpath) {
		int fd;
		fd = open(pidpath, O_CREAT | O_TRUNC | O_WRONLY, 0644);
		if (fd < 0) {
			perror("open");
			(void)fprintf(stderr, "Unable to create %s\n", pidpath);
			exit(EXIT_FAILURE);
		}
		dprintf(fd, "%zd", getpid());
		close(fd);
		free(pidpath);
	}

	info = (vtun_info_t *)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	w.tv_sec = 30;
	w.tv_nsec = 0;
	vtun_info_init(info, &conf, &w);

	return vtun_main(info);
}
