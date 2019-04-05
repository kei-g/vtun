#ifndef __include_client_h__
#define __include_client_h__

#include "vtun.h"

int vtun_client_xfer_p2l(vtun_info_t *info, const struct sockaddr_in *addr);

#endif /* __include_client_h__ */
