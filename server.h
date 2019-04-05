#ifndef __include_server_h__
#define __include_server_h__

#include "vtun.h"

int vtun_server_xfer_p2l(vtun_info_t *info, const struct sockaddr_in *addr);

#endif /* __include_server_h__ */
