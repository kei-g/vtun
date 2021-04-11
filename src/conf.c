#include "conf.h"

#include "base64.h"
#include "client.h"
#include "codec.h"
#include "server.h"
#include "ioctl.h"
#include "sig.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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
	strncpy(conf->dev_type, value, sizeof(conf->dev_type));
}

static void vtun_conf_read_ifaddr(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	char *src, *dst, *mask;

	src = strtok_r(value, " ", &dst);
	strncpy(conf->ifa_dst, dst, sizeof(conf->ifa_dst));

	src = strtok_r(src, "/", &mask);
	strncpy(conf->ifa_src, src, sizeof(conf->ifa_src));

	errno = 0;
	conf->ifa_mask = strtol(mask, NULL, 10);
	if (errno) {
		perror("strtol");
		exit(1);
	}
}

static void vtun_conf_read_iv(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	base64_t b;

	b = base64_alloc();
	base64_decode(b, conf->iv, value);
	base64_free(&b);
}

static void vtun_conf_read_key(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	base64_t b;

	b = base64_alloc();
	base64_decode(b, conf->key, value);
	base64_free(&b);
}

static void vtun_conf_read_route(conf, value)
	vtun_conf_t *conf;
	char *value;
{
	vtun_route_t *r;

	r = (vtun_route_t *)malloc(sizeof(*r));
	if (!r) {
		perror("malloc");
		exit(1);
	}

	strncpy(r->dst, value, sizeof(r->dst));

	r->next = conf->routes;
	conf->routes = r;
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
	{ "iv", vtun_conf_read_iv },
	{ "key", vtun_conf_read_key },
	{ "route", vtun_conf_read_route },
	{ NULL, NULL },
};

void vtun_conf_init(conf, path)
	vtun_conf_t *conf;
	const char *path;
{
	int fd;
	struct stat st;
	char *buf, *lp, *t, *key, *value, dev_name[24];
	ssize_t len;
	const vtun_conf_read_t *c;
	vtun_route_t *r, *next;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open");
		(void)fprintf(stderr, "Unable to open %s\n", path);
		exit(1);
	}

	if (fstat(fd, &st) < 0) {
		perror("fstat");
		(void)fprintf(stderr, "Unable to stat %s\n", path);
		exit(1);
	}

	buf = malloc(st.st_size + 1);
	if (!buf) {
		perror("malloc");
		(void)fprintf(stderr, "Unable to read configurations from %s\n", path);
		exit(1);
	}
	buf[st.st_size] = '\0';

	if ((len = read(fd, buf, st.st_size)) < 0) {
		perror("read");
		(void)fprintf(stderr, "Unable to read configurations from %s\n", path);
		exit(1);
	}

	close(fd);

	memset(conf, 0, sizeof(*conf));
	conf->dev = -1;
	conf->sock = -1;
	conf->mode = VTUN_MODE_UNSPECIFIED;

	for (lp = strtok_r(buf, "\n", &t); lp; lp = strtok_r(NULL, "\n", &t)) {
		key = strtok_r(lp, "=", &value);
		for (c = handlers; c->name; c++)
			if (strcmp(key, c->name) == 0) {
				(*c->func)(conf, value);
				break;
			}
	}

	free(buf);

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

#if defined(__FreeBSD__)
	ioctl_create_interface(conf->dev_type, conf->ifr_name);
	vtun_sig_init();
	vtun_sig_add_interface_by_name(conf->ifr_name);
	ioctl_add_ifaddr(conf->ifr_name, conf->ifa_src,
		conf->ifa_mask, conf->ifa_dst);

	sprintf(dev_name, "/dev/%s", conf->ifr_name);
	if ((conf->dev = open(dev_name, O_RDWR)) < 0) {
		perror("open");
		(void)fprintf(stderr, "Unable to open %s\n", dev_name);
		exit(1);
	}
#elif defined(__linux__)
	conf->dev = ioctl_create_interface(conf->dev_type, dev_name);
	*conf->ifr_name = '\0';
	ioctl_add_ifaddr(dev_name, conf->ifa_src,
		conf->ifa_mask, conf->ifa_dst);
#endif
	vtun_sig_add_interface_by_device(conf->dev);

	for (r = conf->routes; r; r = next) {
		next = r->next;
		ioctl_add_route(r->dst, conf->ifa_dst);
		free(r);
	}
	conf->routes = NULL;
}
