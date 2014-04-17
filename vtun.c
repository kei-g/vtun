#include "vtun.h"
#include "conf.h"

vtun_info_t vtun_init(path)
	const char *path;
{
	vtun_info_t info;

	info = (vtun_info_t)malloc(sizeof(*info));
	if (!info) {
		perror("malloc");
		exit(1);
	}

	info->local = -1;
	info->mode = VTUN_MODE_UNSPECIFIED;
	info->peer = -1;
	memset(&info->server, 0, sizeof(info->server));

	vtun_conf_read(info, path);

	return (info);
}

void vtun_dump_iphdr(info)
	vtun_info_t info;
{
#ifdef DEBUG
	const struct ip *const iphdr = &info->iphdr;
	(void)printf("LEN=%d,ID=%d,OFF=%d,TTL=%d,PROTO=%d,%s => %s\n",
		ntohs(iphdr->ip_len), iphdr->ip_id,
		ntohs(iphdr->ip_off), iphdr->ip_ttl, iphdr->ip_p,
		inet_ntoa_r(iphdr->ip_src, info->name1, sizeof(info->name1)),
		inet_ntoa_r(iphdr->ip_dst, info->name2, sizeof(info->name2)));
#endif
}
