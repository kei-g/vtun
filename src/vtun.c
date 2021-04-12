#include "vtun.h"

#include "base64.h"
#include "codec.h"
#include "conf.h"
#include "xfer.h"

#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__FreeBSD__)
#include <sys/event.h>
#elif defined(__linux__)
#include <sys/epoll.h>
#endif

#include <unistd.h>

static void vtun_info_init(vtun_info_t *info, const vtun_conf_t *conf, struct timespec *w)
{
	info->addr = conf->addr;
	info->dev = conf->dev;
	strncpy(info->devname, conf->ifr_name, sizeof(info->devname));
	info->ignore = conf->mode == VTUN_MODE_SERVER;
	info->sock = conf->sock;
	info->keepalive = conf->mode == VTUN_MODE_CLIENT ? w : NULL;
	info->xfer_p2l = conf->xfer_p2l;

	if (conf->mode == VTUN_MODE_CLIENT)
		vtun_xfer_keepalive(info);

	info->dec = EVP_CIPHER_CTX_new();
	info->enc = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(info->dec);
	EVP_CIPHER_CTX_init(info->enc);
	EVP_DecryptInit_ex(info->dec, EVP_aes_256_gcm(),
		NULL, conf->key, conf->iv);
	EVP_EncryptInit_ex(info->enc, EVP_aes_256_gcm(),
		NULL, conf->key, conf->iv);
}

static int vtun_main(vtun_info_t *info)
{
#if defined(__FreeBSD__)
	int kq, n = 0;
	if ((kq = kqueue()) < 0) {
		perror("kqueue");
		exit(1);
	}

	struct kevent kev[2];
	EV_SET(&kev[n++], info->dev, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&kev[n++], info->sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
	for (;;) {
		if ((n = kevent(kq, kev, n, kev, 1, info->keepalive)) < 0) {
			perror("kevent");
			exit(1);
		}
		void (*func)(vtun_info_t *info);
		if (n == 0)
			func = vtun_xfer_keepalive;
		else
			func = kev->ident == info->dev ? vtun_xfer_l2p : vtun_xfer_p2l;
		(*func)(info);
		n = 0;
	}
#elif defined(__linux__)
	int epl, n = 0;
	if ((epl = epoll_create1(0)) < 0) {
		perror("epoll_create1");
		exit(1);
	}

	struct epoll_event eev[2];
	eev[n].data.fd = info->dev;
	eev[n].events = EPOLLIN;
	if (epoll_ctl(epl, EPOLL_CTL_ADD, info->dev, &eev[n++]) < 0) {
		perror("epoll_ctl(EPOLL_CTL_ADD)");
		exit(1);
	}
	eev[n].data.fd = info->sock;
	eev[n].events = EPOLLIN;
	if (epoll_ctl(epl, EPOLL_CTL_ADD, info->sock, &eev[n++]) < 0) {
		perror("epoll_ctl(EPOLL_CTL_ADD)");
		exit(1);
	}
	int timeout = info->keepalive ? (info->keepalive->tv_sec * 1000 + info->keepalive->tv_nsec / 1000000) : -1;
	for (;;) {
		int r;
		if ((r = epoll_wait(epl, eev, n, timeout)) < 0) {
			perror("epoll_wait");
			exit(1);
		}
		void (*func)(vtun_info_t *info);
		if (r == 0)
			func = vtun_xfer_keepalive;
		else
			func = eev->data.fd == info->dev ? vtun_xfer_l2p : vtun_xfer_p2l;
		(*func)(info);
	}
#endif

	return 0;
}

static int vtun_bn_generate_key(void)
{
	unsigned char iv[12], key[32];
	vtun_generate_iv(iv);
	vtun_generate_key(key);

	base64_t b = base64_alloc();
	char *msg = base64_encode(b, iv, sizeof(iv));
	printf("iv: %s\n", msg);
	free(msg);
	msg = base64_encode(b, key, sizeof(key));
	printf("key: %s\n", msg);
	free(msg);
	base64_free(&b);

	return 0;
}

int main(int argc, char *argv[])
{
	char *bn = basename(*argv);
	if (strcmp(bn, "vtun-keygen") == 0)
		return vtun_bn_generate_key();

	int opt, idx, verbose = 0;
	struct option opts[] = {
		{ "background", no_argument, NULL, 'b' },
		{ "conffile", required_argument, NULL, 'c' },
		{ "pidfile", required_argument, NULL, 'p' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "version", no_argument, NULL, 'V' },
	};
	pid_t pid;
	char *confpath = NULL, *pidpath = NULL;
	while ((opt = getopt_long(argc, argv, "Vbc:p:v", opts, &idx)) != -1)
		switch (opt) {
		case 'V':
			printf("%s version 1.0.0\n", bn);
			exit(EXIT_SUCCESS);
			break;
		case 'b':
			if ((pid = fork()) < 0) {
				perror("fork");
				exit(EXIT_FAILURE);
			}
			if (pid != 0)
				return (0);
			break;
		case 'c':
			confpath = strdup(optarg);
			break;
		case 'p':
			pidpath = strdup(optarg);
			break;
		case 'v':
			verbose++;
			break;
		}

	if (!confpath)
		confpath = strdup("/usr/local/etc/vtun/vtun.conf");
	vtun_conf_t conf;
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
		dprintf(fd, "%d", getpid());
		close(fd);
		free(pidpath);
	}

	vtun_info_t *info = malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	info->verbose = verbose;

	struct timespec w = {.tv_sec = 30, .tv_nsec = 0};
	vtun_info_init(info, &conf, &w);

	return vtun_main(info);
}
