#include "vtun.h"
#include "ioctl.h"

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define INET(p, a) \
	memset(p, 0, sizeof(struct sockaddr_in)); \
	((struct sockaddr_in *)(p))->sin_len = sizeof(struct sockaddr_in); \
	((struct sockaddr_in *)(p))->sin_family = AF_INET; \
	((struct sockaddr_in *)(p))->sin_addr.s_addr = (a);

#define MASK2ADDR(n) htonl(0xffffffff ^ (n < 32 ? ((1 << (32 - n)) - 1) : 0))

void ioctl_add_ifaddr(ifr_name, src, netmask, dst)
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

	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifra_name, ifr_name, sizeof(ifr.ifra_name));
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
	pid_t pid;
	int status;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		status = execl("/sbin/route", "-n", "add", dst, gw, NULL);
		if (status < 0)
			perror("execl");
		exit(status);
	} else
		waitpid(pid, &status, 0);
}

void ioctl_create_interface(dev_type, ifr_name)
	const char *dev_type;
	char *ifr_name;
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
	strcpy(ifr_name, ifr.ifr_name);

	close(sock);
}

void ioctl_destroy_interface(ifr_name)
	const char *ifr_name;
{
	int sock;
	struct ifreq ifr;

	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		return;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifr_name, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCIFDESTROY, &ifr) < 0) {
		perror("ioctl SIOCIFDESTROY");
		return;
	}

	close(sock);
}
