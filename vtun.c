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
#include <sys/event.h>
#include <sys/param.h>
#include <unistd.h>

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
	unsigned char iv[12], key[32];
	base64_t b;
	char *msg;

	vtun_generate_iv(iv);
	vtun_generate_key(key);

	b = base64_alloc();
	msg = base64_encode(b, iv, sizeof(iv));
	printf("iv: %s\n", msg);
	free(msg);
	msg = base64_encode(b, key, sizeof(key));
	printf("key: %s\n", msg);
	free(msg);
	base64_free(&b);

	return (0);
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	char *bn, *confpath = NULL, *pidpath = NULL;
	int opt;
	pid_t pid;
	vtun_conf_t conf;
	vtun_info_t *info;
	struct timespec w;

	bn = basename(*argv);
	if (strcmp(bn, "vtun-keygen") == 0)
		return vtun_bn_generate_key();

	while ((opt = getopt(argc, argv, "bc:p:")) != -1)
		switch (opt) {
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
		dprintf(fd, "%d", getpid());
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
