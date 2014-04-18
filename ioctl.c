#include "vtun.h"

#include <net/if.h>
#include <sys/ioctl.h>

#define INET(p, a) \
	((struct sockaddr_in *)(p))->sin_len = sizeof(struct sockaddr_in); \
	((struct sockaddr_in *)(p))->sin_family = AF_INET; \
	((struct sockaddr_in *)(p))->sin_addr.s_addr = (a);

#define MASK2ADDR(n) htonl(0xffffffff ^ (n < 32 ? ((1 << (32 - n)) - 1) : 0))

void vtun_ioctl_add_ifaddr(ifr_name, src, netmask, dst)
	const char *ifr_name;
	const char *src;
	uint32_t netmask;
	const char *dst;
{
	int sock;
	struct ifaliasreq ifr;
	struct sockaddr_in *const addr = (struct sockaddr_in *)&ifr.ifra_addr;
	struct sockaddr_in *const mask = (struct sockaddr_in *)&ifr.ifra_mask;
	struct sockaddr_in *const dest = (struct sockaddr_in *)&ifr.ifra_broadaddr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifra_name, ifr_name);
	INET(addr, inet_addr(src));
	INET(mask, MASK2ADDR(netmask));
	INET(dest, inet_addr(dst));
	if (ioctl(sock, SIOCAIFADDR, &ifr) < 0) {
		perror("ioctl SIOCAIFADDR");
		exit(1);
	}

	close(sock);
}
