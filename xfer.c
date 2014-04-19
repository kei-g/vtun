#include "xfer.h"
#include "3des.h"

#ifdef DEBUG
static void vtun_dump_iphdr(info)
	vtun_info_t *info;
{
	const struct ip *const iphdr = &info->iphdr;
	(void)printf("LEN=%d,ID=%d,OFF=%d,TTL=%d,PROTO=%d,%s => %s\n",
		ntohs(iphdr->ip_len), ntohs(iphdr->ip_id),
		ntohs(iphdr->ip_off), iphdr->ip_ttl, iphdr->ip_p,
		inet_ntoa_r(iphdr->ip_src, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->ip_dst, info->name2, sizeof(info->name2)));
}
#endif

void vtun_xfer_l2p(info)
	vtun_info_t *info;
{
	ssize_t sent;

	if ((info->buflen = read(info->dev, info->buf, sizeof(info->buf))) < 0) {
		perror("read");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are read.\n", info->buflen);
#endif

	if (info->buflen < sizeof(info->iphdr))
		return;

#ifdef DEBUG
	vtun_dump_iphdr(info);
#endif

	if (info->ignore)
		return;

	vtun_3des_encode(info);

	sent = sendto(info->sock, info->buf, info->buflen, 0,
		(struct sockaddr *)&info->addr, sizeof(info->addr));
	if (sent < 0) {
		perror("sendto");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are sent to %s:%d.\n", sent,
		inet_ntoa(info->addr.sin_addr), ntohs(info->addr.sin_port));
#endif
}

void vtun_xfer_p2l(info)
	vtun_info_t *info;
{
	socklen_t addrlen;
	struct sockaddr_in addr;
	ssize_t sent;

	addrlen = sizeof(addr);
	info->buflen = recvfrom(info->sock, info->buf, sizeof(info->buf), 0,
		(struct sockaddr *)&addr, &addrlen);
	if (info->buflen < 0) {
		perror("recvfrom");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are received from %s:%d.\n", info->buflen,
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif

	if (!(*info->xfer_p2l)(info, &addr))
		return;

	vtun_3des_decode(info);
#ifdef DEBUG
	vtun_dump_iphdr(info);
#endif

	if ((sent = write(info->dev, info->buf, info->buflen)) < 0) {
		perror("write");
		exit(1);
	}
#ifdef DEBUG
	(void)printf("%ld bytes are written.\n", sent);
#endif
}
