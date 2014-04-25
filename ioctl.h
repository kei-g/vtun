#ifndef __include_ioctl_h__
#define __include_ioctl_h__

#include <sys/types.h>

void ioctl_add_ifaddr(const char *ifr_name,
	const char *src, uint32_t mask, const char *dst);
void ioctl_add_route(const char *dst, const char *gw);
void ioctl_create_interface(const char *dev_type, char *ifr_name);
void ioctl_destroy_interface(const char *ifr_name);

#endif /* __include_ioctl_h__ */
