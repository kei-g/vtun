#ifndef __include_ioctl_h__
#define __include_ioctl_h__

#include <sys/types.h>

void vtun_ioctl_add_ifaddr(const char *ifr_name,
	const char *src, uint32_t mask, const char *dst);
void vtun_ioctl_create_interface(const char *dev_type, char *ifr_name);
void vtun_ioctl_destroy_interface(const char *ifr_name);

#endif /* __include_ioctl_h__ */
