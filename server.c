#include "server.h"

void vtun_server_xfer_l2p(info)
	vtun_info_t *info;
{
	ssize_t len, sent;

	if ((len = read(info->dev, info->buf, sizeof(info->buf))) < 0) {
		perror("read");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are read.\n", len);
#endif
	if (len == 0)
		return;

	vtun_dump_iphdr(info);

	if (info->addr.sin_family != AF_INET) {
		(void)fprintf(stderr, "peer has not been prepared yet.\n");
		return;
	}

	vtun_3des_encode(info, &len);

	sent = sendto(info->sock, info->buf, len, 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are sent to %s:%d.\n", sent,
		inet_ntoa(info->addr.sin_addr), ntohs(info->addr.sin_port));
#endif
}

void vtun_server_xfer_p2l(info)
	vtun_info_t *info;
{
	socklen_t calen;
	struct sockaddr_in ca;
	ssize_t len, sent;

	calen = sizeof(ca);
	len = recvfrom(info->sock, info->buf, sizeof(info->buf), 0,
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

	info->addr.sin_family = ca.sin_family;
	info->addr.sin_port = ca.sin_port;
	info->addr.sin_addr = ca.sin_addr;

	if ((sent = write(info->dev, info->buf, ntohs(info->iphdr.ip_len))) < 0) {
		perror("write");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are written.\n", sent);
#endif
}
