#ifndef __include_ioctl_h__
#define __include_ioctl_h__

#include <stdint.h>

void ioctl_add_ifaddr(const char *name,
	const char *src, uint32_t mask, const char *dst);

#if defined(__FreeBSD__)
void ioctl_add_route(const char *dst, const char *gw);
void ioctl_create_interface(const char *dev_type, char *ifr_name);
void ioctl_destroy_interface(const char *ifr_name);
#elif defined(__linux__)
void ioctl_add_route(const char *dst, const char *gw);
int ioctl_create_interface(const char *dev_type, char *name);
#endif

#endif /* __include_ioctl_h__ */
