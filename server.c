#include "server.h"

void vtun_server_xfer_l2p(info)
	vtun_info_t info;
{
}

void vtun_server_xfer_p2l(info)
	vtun_info_t info;
{
	socklen_t calen;
	struct sockaddr_in ca;
	ssize_t len, sent;

	calen = sizeof(ca);
	len = recvfrom(info->peer, info->buf, sizeof(info->buf), 0,
		(struct sockaddr *)&ca, &calen);
	if (len < 0) {
		perror("recvfrom");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are received from %s:%d.\n", len,
		inet_ntoa(ca.sin_addr), ntohs(ca.sin_port));
#endif
	if (len == 0)
		return;

	vtun_3des_decode(info, &len);
	vtun_dump_iphdr(info);

	/* XXX: TODO - Register to the table. */

	sent = write(info->local, info->buf, ntohs(info->iphdr.ip_len));
	if (sent < 0) {
		perror("write");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are written.\n", sent);
#endif
}
