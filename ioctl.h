#ifndef __include_ioctl_h__
#define __include_ioctl_h__

void vtun_ioctl_add_ifaddr(const char *ifr_name,
	const char *src, uint32_t mask, const char *dst);

#endif /* __include_ioctl_h__ */
