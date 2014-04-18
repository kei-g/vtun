#include "vtun.h"
#include "conf.h"

void vtun_3des(info, len, enc)
	vtun_info_t *info;
	ssize_t len;
	int enc;
{
	ssize_t i;
	for (i = 0; i < len; i += sizeof(info->temp)) {
		memset(&info->temp, 0, sizeof(info->temp));
		DES_ecb3_encrypt((DES_cblock *)&info->buf[i], &info->temp,
			&info->sched[0], &info->sched[1], &info->sched[2], enc);
		memcpy(&info->buf[i], &info->temp, sizeof(info->temp));
	}
}

void vtun_3des_decode(info, len)
	vtun_info_t *info;
	ssize_t *len;
{
	vtun_3des(info, *len, DES_DECRYPT);
	*len = ntohs(info->iphdr.ip_len);
}

void vtun_3des_encode(info, len)
	vtun_info_t *info;
	ssize_t *len;
{
	*len = (*len + 7) & ~7;
	vtun_3des(info, *len, DES_ENCRYPT);
}

void vtun_dump_iphdr(info)
	vtun_info_t *info;
{
#ifdef DEBUG
	const struct ip *const iphdr = &info->iphdr;
	(void)printf("LEN=%d,ID=%d,OFF=%d,TTL=%d,PROTO=%d,%s => %s\n",
		ntohs(iphdr->ip_len), ntohs(iphdr->ip_id),
		ntohs(iphdr->ip_off), iphdr->ip_ttl, iphdr->ip_p,
		inet_ntoa_r(iphdr->ip_src, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->ip_dst, info->name2, sizeof(info->name2)));
#endif
}

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
	info->xfer_l2p = conf->xfer_l2p;
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
			func = kev->ident == info->dev ? info->xfer_l2p : info->xfer_p2l;
			memset(info->buf, 0, sizeof(info->buf));
			(*func)(info);
		}
	}

	return (0);
}
