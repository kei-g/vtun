#include "conf.h"
#include "client.h"
#include "server.h"

static void vtun_read_address(info, value)
	vtun_info_t info;
	char *value;
{
	char *name, *port;

	name = strtok_r(value, ":", &port);

	if ((info->peer = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	info->addr.sin_family = AF_INET;
	info->addr.sin_port = htons(atoi(port));
	info->addr.sin_addr.s_addr = inet_addr(name);
}

static void vtun_read_bind(info, value)
	vtun_info_t info;
	char *value;
{
	if (info->mode == VTUN_MODE_CLIENT) {
		(void)fprintf(stderr, "Unable to bind after connect.\n");
		exit(1);
	}
	info->mode = VTUN_MODE_SERVER;
	info->xfer_l2p = vtun_server_xfer_l2p;
	info->xfer_p2l = vtun_server_xfer_p2l;

	vtun_read_address(info, value);

	if (bind(info->peer, (struct sockaddr *)&info->addr,
		sizeof(info->addr)) < 0) {
		perror("bind");
		exit(1);
	}

	info->addr.sin_family = AF_INET6;
}

static void vtun_read_connect(info, value)
	vtun_info_t info;
	char *value;
{
	if (info->mode == VTUN_MODE_SERVER) {
		(void)fprintf(stderr, "Unable to connect after bind.\n");
		exit(1);
	}
	info->mode = VTUN_MODE_CLIENT;
	info->xfer_l2p = vtun_client_xfer_l2p;
	info->xfer_p2l = vtun_client_xfer_p2l;

	vtun_read_address(info, value);
}

static void vtun_read_device(info, value)
	vtun_info_t info;
	char *value;
{
	if ((info->local = open(value, O_RDWR)) < 0) {
		perror("open");
		exit(1);
	}
}

static void vtun_read_key(info, value)
	vtun_info_t info;
	char *value;
{
	DES_cblock key[3];
	char *t;
	long v;
	uint8_t *p = (uint8_t *)key;
	ssize_t i;

	memset(key, 0, sizeof(key));
	for (t = value; *t;) {
		errno = 0;
		v = strtol(t, &t, 16);
		if (errno != 0) {
			perror("strtol");
			exit(1);
		}
		*p++ = (uint8_t)v;
	}

	for (i = 0; i < sizeof(key) / sizeof(key[0]); i++)
		DES_set_key_checked(&key[i], &info->sched[i]);
}

typedef struct {
	const char *name;
	void (*func)(vtun_info_t, char *);
} vtun_read_t;

static const vtun_read_t handlers[] = {
	{ "bind", vtun_read_bind },
	{ "connect", vtun_read_connect },
	{ "device", vtun_read_device },
	{ "key", vtun_read_key },
	{ NULL, NULL },
};

void vtun_conf_read(info, path)
	vtun_info_t info;
	const char *path;
{
	int fd;
	char buf[128], *lp, *t, *key, *value;
	ssize_t len;
	const vtun_read_t *r;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open");
		exit(1);
	}

	memset(buf, 0, sizeof(buf));
	if ((len = read(fd, buf, sizeof(buf) - 1)) < 0) {
		perror("read");
		exit(1);
	}

	close(fd);

	info->local = -1;
	info->mode = VTUN_MODE_UNSPECIFIED;
	info->peer = -1;
	memset(&info->addr, 0, sizeof(info->addr));
	info->xfer_l2p = NULL;
	info->xfer_p2l = NULL;

	for (lp = strtok_r(buf, "\n", &t); lp; lp = strtok_r(NULL, "\n", &t)) {
		key = strtok_r(lp, "=", &value);
		for (r = handlers; r->name; r++)
			if (strcmp(key, r->name) == 0) {
				(*r->func)(info, value);
				break;
			}
	}

	if (info->local < 0) {
		fprintf(stderr, "No device is specified.\n");
		exit(1);
	}
	if (!info->xfer_l2p || !info->xfer_p2l) {
		fprintf(stderr, "Neither bind nor connect is specified.\n");
		exit(1);
	}
}
