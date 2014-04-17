#include "vtun.h"

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

	info->server.sin_family = AF_INET;
	info->server.sin_port = htons(atoi(port));
	info->server.sin_addr.s_addr = inet_addr(name);
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

	vtun_read_address(info, value);

	if (bind(info->peer, (struct sockaddr *)&info->server,
		sizeof(info->server)) < 0) {
		perror("bind");
		exit(1);
	}
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

vtun_info_t vtun_init(path)
	const char *path;
{
	int fd;
	char buf[128], *lp, *t, *k, *v;
	vtun_info_t info;
	const vtun_read_t *r;

	if ((fd = open(path, O_RDWR)) < 0) {
		perror("open");
		exit(1);
	}
	memset(buf, 0, sizeof(buf));
	if (read(fd, buf, sizeof(buf)) < 0) {
		perror("read");
		exit(1);
	}
	close(fd);

	info = (vtun_info_t)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	info->local = -1;
	info->mode = VTUN_MODE_UNSPECIFIED;
	info->peer = -1;
	memset(&info->server, 0, sizeof(info->server));

	for (lp = strtok_r(buf, "\n", &t); lp; lp = strtok_r(NULL, "\n", &t)) {
		k = strtok_r(lp, "=", &v);
		for (r = handlers; r->name; r++)
			if (strcmp(k, r->name) == 0) {
				(*r->func)(info, v);
				break;
			}
	}

	return (info);
}

void vtun_dump_iphdr(info)
	vtun_info_t info;
{
#ifdef DEBUG
	const struct ip *const iphdr = &info->iphdr;
	(void)printf("LEN=%d,ID=%d,OFF=%d,TTL=%d,PROTO=%d,%s => %s\n",
		ntohs(iphdr->ip_len), iphdr->ip_id,
		ntohs(iphdr->ip_off), iphdr->ip_ttl, iphdr->ip_p,
		inet_ntoa_r(iphdr->ip_src, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->ip_dst, info->name2, sizeof(info->name2)));
#endif
}
