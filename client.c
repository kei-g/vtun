#include "vtun.h"

int main(argc, argv)
	int argc;
	char *argv[];
{
	vtun_info_t info;
	ssize_t len, i, sent;

	info = vtun_init("vtun.conf");
	for (;;) {
		memset(info->buf, 0, sizeof(info->buf));
		len = read(info->local, info->buf, sizeof(info->buf));
		if (len < 0) {
			perror("read");
			break;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are read.\n", len);
#endif
		if (len == 0)
			break;
		vtun_dump_iphdr(info);
		len = (len + 7) & ~7;
		for (i = 0; i < len; i += sizeof(info->temp)) {
			memset(&info->temp, 0, sizeof(info->temp));
			DES_ecb3_encrypt((DES_cblock *)&info->buf[i],
				&info->temp,
				&info->sched[0],
				&info->sched[1],
				&info->sched[2],
				DES_ENCRYPT);
			memcpy(&info->buf[i], &info->temp, sizeof(info->temp));
		}
		sent = sendto(info->peer, info->buf, len, 0,
			(struct sockaddr *)&info->server,
			sizeof(info->server));
		if (sent < 0) {
			perror("sendto");
			continue;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are sent.\n", sent);
#endif
	}
	return (0);
}
