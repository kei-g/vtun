#include "server.h"

void vtun_server(info)
	vtun_info_t info;
{
	struct sockaddr_in ca;
	socklen_t calen;
	ssize_t len, i, sent;
	for (;;) {
		memset(info->buf, 0, sizeof(info->buf));
		calen = sizeof(ca);
		len = recvfrom(info->peer, info->buf, sizeof(info->buf), 0,
			(struct sockaddr *)&ca, &calen);
		if (len < 0) {
			perror("recvfrom");
			break;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are received from %s:%d.\n", len,
			inet_ntoa(ca.sin_addr), ntohs(ca.sin_port));
#endif
		if (len == 0)
			break;
		for (i = 0; i < len; i += sizeof(info->temp)) {
			memset(&info->temp, 0, sizeof(info->temp));
			DES_ecb3_encrypt((DES_cblock *)&info->buf[i],
				&info->temp,
				&info->sched[0],
				&info->sched[1],
				&info->sched[2],
				DES_DECRYPT);
			memcpy(&info->buf[i], &info->temp, sizeof(info->temp));
		}
		vtun_dump_iphdr(info);
		sent = write(info->local, info->buf, ntohs(info->iphdr.ip_len));
		if (sent < 0) {
			perror("write");
			continue;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are written.\n", sent);
#endif
	}
}
