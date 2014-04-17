#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <openssl/des.h>

static void vtun_key_init(sched)
	DES_key_schedule sched[3];
{
	uint8_t privkey[] = {
		0xD0, 0x89, 0xB5, 0x31, 0xA7, 0x70, 0xE5, 0xAE,
		0x1C, 0x26, 0x0E, 0x54, 0xAD, 0x4A, 0xC1, 0x6B,
		0x73, 0x45, 0x57, 0x7A, 0x92, 0x29, 0x94, 0xFB,
	};
	DES_cblock *const key = (DES_cblock *)privkey;
	ssize_t i;
	for (i = 0; i < sizeof(privkey) / sizeof(*key); i++)
		DES_set_key_checked(&key[i], &sched[i]);
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	int fd, td;
	struct sockaddr_in sa;
	DES_key_schedule sched[3];
	uint8_t buf[65536], temp[8];
	ssize_t len, i, sent;
#ifdef DEBUG
	const struct ip *const iphdr = (const struct ip *)buf;
	char name1[32], name2[32];
#endif
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (1);
	}
	if ((td = open("/dev/tun0", O_RDWR)) < 0) {
		perror("open");
		return (1);
	}
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(61000);
	sa.sin_addr.s_addr = inet_addr("49.212.186.119");
	vtun_key_init(sched);
	for (;;) {
		memset(buf, 0, sizeof(buf));
		len = read(td, buf, sizeof(buf));
		if (len < 0) {
			perror("read");
			break;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are read.\n", len);
#endif
		if (len == 0)
			break;
#ifdef DEBUG
		(void)printf("LEN=%d,ID=%d,OFF=%d,TTL=%d,PROTO=%d,%s => %s\n",
			ntohs(iphdr->ip_len), iphdr->ip_id,
			ntohs(iphdr->ip_off), iphdr->ip_ttl, iphdr->ip_p,
			inet_ntoa_r(iphdr->ip_src, name1, sizeof(sa)),
			inet_ntoa_r(iphdr->ip_dst, name2, sizeof(sa)));
#endif
		len = (len + 7) & ~7;
		for (i = 0; i < len; i += sizeof(temp)) {
			memset(temp, 0, sizeof(temp));
			DES_ecb3_encrypt((DES_cblock *)&buf[i],
				(DES_cblock *)temp,
				&sched[0], &sched[1], &sched[2],
				DES_ENCRYPT);
			memcpy(&buf[i], temp, sizeof(temp));
		}
		sent = sendto(fd, buf, len, 0,
			(struct sockaddr *)&sa, sizeof(sa));
		if (sent < 0) {
			perror("sendto");
			continue;
		}
#ifdef DEBUG
		(void)printf("%ld bytes are sent.\n", sent);
#endif
	}
	return (0);
}
