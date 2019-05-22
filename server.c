#include "server.h"

int vtun_server_xfer_p2l(info, addr)
	vtun_info_t *info;
	const struct sockaddr_in *addr;
{
	if (info->obj.cmd != 1)
		return 0;
	info->addr.sin_family = addr->sin_family;
	info->addr.sin_port = addr->sin_port;
	info->addr.sin_addr = addr->sin_addr;
	info->ignore = 0;
	return sizeof(info->obj.cmd) + sizeof(info->obj.iphdr) <= info->buflen;
}
