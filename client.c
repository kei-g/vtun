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
		(struct sockaddr *)&info->server,
		sizeof(info->server));
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
}
