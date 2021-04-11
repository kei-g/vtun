#include "vtun.h"
#include "ioctl.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#define INET(p, a) \
	memset(p, 0, sizeof(struct sockaddr_in)); \
	((struct sockaddr_in *)(p))->sin_len = sizeof(struct sockaddr_in); \
	((struct sockaddr_in *)(p))->sin_family = AF_INET; \
	((struct sockaddr_in *)(p))->sin_addr.s_addr = (a);
#elif defined(__linux__)
#define INET(p, a) \
	memset(p, 0, sizeof(struct sockaddr_in)); \
	((struct sockaddr_in *)(p))->sin_family = AF_INET; \
	((struct sockaddr_in *)(p))->sin_addr.s_addr = (a);
#endif

#define MASK2ADDR(n) htonl(0xffffffff ^ (n < 32 ? ((1 << (32 - n)) - 1) : 0))

void ioctl_add_ifaddr(name, src, netmask, dst)
	const char *name;
	const char *src;
	uint32_t netmask;
	const char *dst;
{
	int sock;
	struct ifaliasreq ifr;
	struct sockaddr_in *const addr = (struct sockaddr_in *)&ifr.ifra_addr;
	struct sockaddr_in *const mask = (struct sockaddr_in *)&ifr.ifra_mask;
	struct sockaddr_in *const dest = (struct sockaddr_in *)&ifr.ifra_broadaddr;

	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifra_name, name, sizeof(ifr.ifra_name));
	INET(addr, inet_addr(src));
	INET(mask, MASK2ADDR(netmask));
	INET(dest, inet_addr(dst));
	if (ioctl(sock, SIOCAIFADDR, &ifr) < 0) {
		perror("ioctl SIOCAIFADDR");
		exit(1);
	}

	close(sock);
}

void ioctl_add_route(dst, gw)
	const char *dst;
	const char *gw;
{
	char tmp[32], *dstnet, *maskstr, *ep;
	uint8_t mask;
	int sock;

	strcpy(tmp, dst);
	dstnet = strtok_r(tmp, "/", &maskstr);
	mask = (uint8_t)strtoul(maskstr, &ep, 10);

#if defined(__FreeBSD__)
	struct {
		struct rt_msghdr hdr;
		struct sockaddr_in rtm_addrs[3];
	} msg;

	if ((sock = socket(PF_ROUTE, SOCK_RAW, 0)) < 0) {
		perror("unable to add route, socket(PF_ROUTE)");
		exit(1);
	}

	memset(&msg, 0, sizeof(msg));

	msg.hdr.rtm_msglen = sizeof(msg);
	msg.hdr.rtm_version = RTM_VERSION;
	msg.hdr.rtm_type = RTM_ADD;
	msg.hdr.rtm_flags = RTF_GATEWAY | RTF_STATIC | RTF_UP;
	msg.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
	msg.hdr.rtm_seq = 1;

	INET(&msg.rtm_addrs[RTAX_DST], inet_addr(dstnet));
	INET(&msg.rtm_addrs[RTAX_GATEWAY], inet_addr(gw));
	INET(&msg.rtm_addrs[RTAX_NETMASK], MASK2ADDR(mask));

	if (write(sock, &msg, sizeof(msg)) < 0) {
		perror("failed to add route, write(PF_ROUTE)");
		exit(1);
	}
#elif defined(__linux__)
	struct rtentry rte;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("unable to add route, socket(AF_INET, SOCK_DGRAM, 0)");
		exit(1);
	}

	INET(&rte.rt_dst, inet_addr(dstnet));
	INET(&rte.rt_gateway, inet_addr(gw));
	INET(&rte.rt_genmask, MASK2ADDR(mask));
	rte.rt_flags = RTF_GATEWAY | RTF_STATIC | RTF_UP;
	rte.rt_metric = htons(1);
	if (ioctl(sock, SIOCADDRT, &rte) < 0) {
		perror("failed to add route, ioctl(SIOCADDRT)");
		exit(1);
	}
#endif

	close(sock);
}

void ioctl_create_interface(dev_type, name)
	const char *dev_type;
	char *name;
{
	int sock;
	struct ifreq ifr;

	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, dev_type, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCIFCREATE2, &ifr) < 0) {
		perror("ioctl SIOCIFCREATE2");
		exit(1);
	}
	strcpy(name, ifr.ifr_name);

	close(sock);
}

void ioctl_destroy_interface(name)
	const char *name;
{
	int sock;
	struct ifreq ifr;

	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		return;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCIFDESTROY, &ifr) < 0) {
		perror("ioctl SIOCIFDESTROY");
		return;
	}

	close(sock);
}
