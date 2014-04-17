#include "vtun.h"
#include "conf.h"

void vtun_3des_decode(info, len)
	vtun_info_t info;
	ssize_t *len;
{
	ssize_t i;
	for (i = 0; i < *len; i += sizeof(info->temp)) {
		memset(&info->temp, 0, sizeof(info->temp));
		DES_ecb3_encrypt((DES_cblock *)&info->buf[i], &info->temp,
			&info->sched[0],
			&info->sched[1],
			&info->sched[2],
			DES_DECRYPT);
		memcpy(&info->buf[i], &info->temp, sizeof(info->temp));
	}
	*len = ntohs(info->iphdr.ip_len);
}

void vtun_3des_encode(info, len)
	vtun_info_t info;
	ssize_t *len;
{
	ssize_t i;
	*len = (*len + 7) & ~7;
	for (i = 0; i < *len; i += sizeof(info->temp)) {
		memset(&info->temp, 0, sizeof(info->temp));
		DES_ecb3_encrypt((DES_cblock *)&info->buf[i], &info->temp,
			&info->sched[0],
			&info->sched[1],
			&info->sched[2],
			DES_ENCRYPT);
		memcpy(&info->buf[i], &info->temp, sizeof(info->temp));
	}
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

int main(argc, argv)
	int argc;
	char *argv[];
{
	vtun_info_t info;
	int kq;
	struct kevent kev[2];
	void (*func)(vtun_info_t);

	info = (vtun_info_t)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	if ((kq = kqueue()) < 0) {
		perror("kqueue");
		exit(1);
	}

	vtun_conf_read(info, "vtun.conf");

	EV_SET(&kev[0], info->local, EVFILT_READ, EV_ADD, 0, 0, NULL);
	EV_SET(&kev[1], info->peer, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(kq, kev, sizeof(kev) / sizeof(kev[0]), NULL, 0, NULL) < 0) {
		perror("kevent");
		exit(1);
	}

	for (;;) {
		if (kevent(kq, NULL, 0, kev, 1, NULL) < 0) {
			perror("kevent");
			exit(1);
		}
		func = kev->ident == info->local ? info->xfer_l2p : info->xfer_p2l;
		memset(info->buf, 0, sizeof(info->buf));
		(*func)(info);
	}

	return (0);
}
