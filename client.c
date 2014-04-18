#include "client.h"

void vtun_client_xfer_l2p(info)
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

void vtun_client_xfer_p2l(info)
	vtun_info_t *info;
{
	socklen_t addrlen;
	struct sockaddr_in addr;
	ssize_t len, sent;

	addrlen = sizeof(addr);
	len = recvfrom(info->sock, info->buf, sizeof(info->buf), 0,
		(struct sockaddr *)&addr, &addrlen);
	if (len < 0) {
		perror("recvfrom");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are received from %s:%d.\n", len,
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif

	if (addr.sin_addr.s_addr != info->addr.sin_addr.s_addr ||
		addr.sin_port != info->addr.sin_port)
		return;

	vtun_3des_decode(info, &len);
	vtun_dump_iphdr(info);

	if ((sent = write(info->dev, info->buf, len)) < 0) {
		perror("write");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are written.\n", sent);
#endif
}
