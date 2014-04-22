#ifndef __include_xfer_h__
#define __include_xfer_h__

#include "vtun.h"

void vtun_xfer_keepalive(vtun_info_t *info);
void vtun_xfer_l2p(vtun_info_t *info);
void vtun_xfer_p2l(vtun_info_t *info);
void vtun_xfer_raw(vtun_info_t *info, const void *msg, size_t len);

#endif /* __include_xfer_h__ */
