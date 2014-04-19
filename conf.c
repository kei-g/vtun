#include "conf.h"

#include "client.h"
#include "server.h"
#include "ioctl.h"

static void vtun_conf_read_address(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	char *name, *port;

	name = strtok_r(value, ":", &port);

	if ((conf->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	conf->addr.sin_family = AF_INET;
	conf->addr.sin_port = htons(atoi(port));
	conf->addr.sin_addr.s_addr = inet_addr(name);
}

static void vtun_conf_read_bind(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	struct sockaddr_in *addr = &conf->addr;

	if (conf->mode == VTUN_MODE_CLIENT) {
		(void)fprintf(stderr, "Unable to bind after connect.\n");
		exit(1);
	}
	conf->mode = VTUN_MODE_SERVER;
	conf->xfer_p2l = vtun_server_xfer_p2l;

	vtun_conf_read_address(conf, value);

	if (bind(conf->sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
		perror("bind");
		exit(1);
	}
}

static void vtun_conf_read_connect(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	if (conf->mode == VTUN_MODE_SERVER) {
		(void)fprintf(stderr, "Unable to connect after bind.\n");
		exit(1);
	}
	conf->mode = VTUN_MODE_CLIENT;
	conf->xfer_p2l = vtun_client_xfer_p2l;

	vtun_conf_read_address(conf, value);
}

static void vtun_conf_read_device(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	strcpy(conf->dev_type, value);
}

static void vtun_conf_read_ifaddr(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	char *src, *dst, *mask;

	src = strtok_r(value, " ", &dst);
	strcpy(conf->ifa_dst, dst);

	src = strtok_r(src, "/", &mask);
	strcpy(conf->ifa_src, src);

	errno = 0;
	conf->ifa_mask = strtol(mask, NULL, 10);
	if (errno) {
		perror("strtol");
		exit(1);
	}
}

static void vtun_conf_read_key(conf, value)
	vtun_conf_t *conf;
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
		DES_set_key_checked(&key[i], &conf->sched[i]);
}

typedef struct {
	const char *name;
	void (*func)(vtun_conf_t *conf, char *value);
} vtun_conf_read_t;

static const vtun_conf_read_t handlers[] = {
	{ "bind", vtun_conf_read_bind },
	{ "connect", vtun_conf_read_connect },
	{ "device", vtun_conf_read_device },
	{ "ifaddr", vtun_conf_read_ifaddr },
	{ "key", vtun_conf_read_key },
	{ NULL, NULL },
};

void vtun_conf_init(conf, path)
	vtun_conf_t *conf;
	const char *path;
{
	int fd;
	char buf[512], *lp, *t, *key, *value, dev_name[24];
	ssize_t len;
	const vtun_conf_read_t *r;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open");
		(void)fprintf(stderr, "Unable to open %s\n", path);
		exit(1);
	}

	memset(buf, 0, sizeof(buf));
	if ((len = read(fd, buf, sizeof(buf) - 1)) < 0) {
		perror("read");
		exit(1);
	}

	close(fd);

	memset(conf, 0, sizeof(*conf));
	conf->dev = -1;
	conf->sock = -1;
	conf->mode = VTUN_MODE_UNSPECIFIED;

	for (lp = strtok_r(buf, "\n", &t); lp; lp = strtok_r(NULL, "\n", &t)) {
		key = strtok_r(lp, "=", &value);
		for (r = handlers; r->name; r++)
			if (strcmp(key, r->name) == 0) {
				(*r->func)(conf, value);
				break;
			}
	}

	if (!*conf->dev_type) {
		fprintf(stderr, "No device is specified.\n");
		exit(1);
	}
	if (!*conf->ifa_src || !*conf->ifa_dst) {
		fprintf(stderr, "No ifaddr is specified.\n");
		exit(1);
	}
	if (conf->sock < 0) {
		fprintf(stderr, "Neither bind nor connect is specified.\n");
		exit(1);
	}

	vtun_ioctl_create_interface(conf->dev_type, conf->ifr_name);
	vtun_ioctl_add_ifaddr(conf->ifr_name, conf->ifa_src,
		conf->ifa_mask, conf->ifa_dst);

	sprintf(dev_name, "/dev/%s", conf->ifr_name);
	if ((conf->dev = open(dev_name, O_RDWR)) < 0) {
		perror("open");
		(void)fprintf(stderr, "Unable to open %s\n", dev_name);
		exit(1);
	}
}
