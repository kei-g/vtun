#include "codec.h"
#include "xfer.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__)
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <string.h>
#endif
#include <sys/socket.h>
#include <unistd.h>

#if defined(__linux__)
static char *inet_ntoa_r(uint32_t addr, char *dest, size_t destlen)
{
	struct in_addr a = {.s_addr = addr};
	strncpy(dest, inet_ntoa(a), destlen);
	return dest;
}
#endif

static void vtun_dump_iphdr(vtun_info_t *info)
{
#if defined(__FreeBSD__)
	const struct ip *const iphdr = &info->tun.iphdr;
	(void)printf("LEN=%u,ID=%u,F=%0x,OFF=%u,TTL=%u,PROTO=%u,%s => %s\n",
		ntohs(iphdr->ip_len), ntohs(iphdr->ip_id),
		ntohs(iphdr->ip_off) >> 13, ntohs(iphdr->ip_off) & 0x1fff,
		iphdr->ip_ttl, iphdr->ip_p,
		inet_ntoa_r(iphdr->ip_src, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->ip_dst, info->name2, sizeof(info->name2)));
#elif defined(__linux__)
	const uint16_t ethtype = ntohs(info->tun.ethtype);
	if (ethtype != ETHERTYPE_IP)
		return;
	const struct iphdr *const iphdr = &info->tun.iphdr;
	(void)printf("LEN=%u,ID=%u,F=%0x,OFF=%u,TTL=%u,PROTO=%u,%s => %s\n",
		ntohs(iphdr->tot_len), ntohs(iphdr->id),
		ntohs(iphdr->frag_off) >> 13, ntohs(iphdr->frag_off) & 0x1fff,
		iphdr->ttl, iphdr->protocol,
		inet_ntoa_r(iphdr->saddr, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->daddr, info->name2, sizeof(info->name2)));
#endif
}

void vtun_xfer_keepalive(vtun_info_t *info)
{
	uint32_t v = 0;
	vtun_xfer_raw(info, &v, sizeof(v));
}

void vtun_xfer_l2p(vtun_info_t *info)
{
	info->buflen = read(info->dev, info->tmp, sizeof(info->tmp));
	if (info->buflen < 0) {
		perror("read");
		exit(1);
	}

#if defined(__linux__)
	if (info->buflen < 4)
		return;
	info->buflen -= 4;
#endif

	if (info->verbose)
		(void)printf("%zd bytes read from %s\n",
			info->buflen, info->devname);

	if (info->buflen < sizeof(info->obj.iphdr))
		return;

	if (info->verbose)
		vtun_dump_iphdr(info);

	if (info->ignore)
		return;

	vtun_encode(info);

	info->obj.cmd = 1;
	info->buflen += sizeof(info->obj.cmd);
	ssize_t sent = sendto(info->sock, info->all, info->buflen, 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
	if (info->verbose)
		(void)printf("%zd bytes are sent to %s:%d.\n", sent,
			inet_ntoa(info->addr.sin_addr), ntohs(info->addr.sin_port));
}

void vtun_xfer_p2l(vtun_info_t *info)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	info->buflen = recvfrom(info->sock, info->all, sizeof(info->all), 0,
		(struct sockaddr *)&addr, &addrlen);
	if (info->buflen < 0) {
		perror("recvfrom");
		exit(1);
	}
	if (info->verbose)
		(void)printf("%zd bytes are received from %s:%d.\n", info->buflen,
			inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	if (info->obj.cmd != 1 || !(*info->xfer_p2l)(info, &addr))
		return;
	info->buflen -= sizeof(info->obj.cmd);

	vtun_decode(info);
	if (info->verbose)
		vtun_dump_iphdr(info);

	ssize_t sent = write(info->dev, info->tmp, info->buflen);
	if (sent < 0) {
		perror("write");
		exit(1);
	}
	if (info->verbose)
		(void)printf("%zd bytes written to %s\n", sent, info->devname);
}

void vtun_xfer_raw(vtun_info_t *info, const void *msg, size_t len)
{
	ssize_t sent = sendto(info->sock, msg, len, 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
}
