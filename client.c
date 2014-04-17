#include "client.h"

void vtun_client_xfer_l2p(info)
	vtun_info_t info;
{
	ssize_t len, sent;

	len = read(info->local, info->buf, sizeof(info->buf));
	if (len < 0) {
		perror("read");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are read.\n", len);
#endif
	if (len == 0)
		return;
	vtun_dump_iphdr(info);

	vtun_3des_encode(info, &len);

	sent = sendto(info->peer, info->buf, len, 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are sent.\n", sent);
#endif
}

void vtun_client_xfer_p2l(info)
	vtun_info_t info;
{
	socklen_t salen;
	struct sockaddr_in sa;
	ssize_t len;

	salen = sizeof(sa);
	len = recvfrom(info->peer, info->buf, sizeof(info->buf), 0,
		(struct sockaddr *)&sa, &salen);
	if (len < 0) {
		perror("recvfrom");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are received from %s:%d.\n", len,
		inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
#endif

	if (sa.sin_addr.s_addr != info->addr.sin_addr.s_addr ||
		sa.sin_port != info->addr.sin_port)
		return;

	vtun_3des_decode(info, &len);
	vtun_dump_iphdr(info);

	len = write(info->local, info->buf, len);
	if (len < 0) {
		perror("write");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are written.\n", len);
#endif
}
