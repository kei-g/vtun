#include "client.h"

int vtun_client_xfer_p2l(info, addr)
	vtun_info_t *info;
	const struct sockaddr_in *addr;
{
	return info->obj.cmd == 1 &&
		addr->sin_addr.s_addr == info->addr.sin_addr.s_addr &&
		addr->sin_port == info->addr.sin_port;
}
