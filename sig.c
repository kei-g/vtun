#include "sig.h"

#include "ioctl.h"
#include "vtun.h"

#include <signal.h>

typedef struct _vtun_sig_interface vtun_sig_interface_t;

struct _vtun_sig_interface {
	vtun_sig_interface_t *next;
	char ifr_name[16];
};

static vtun_sig_interface_t *interfaces = NULL;

static void vtun_sig_handle(sig)
	int sig;
{
	vtun_sig_interface_t *ifr, *next;

	for (ifr = interfaces; ifr; ifr = next) {
		next = ifr->next;
		vtun_ioctl_destroy_interface(ifr->ifr_name);
		free(ifr);
	}

	exit(1);
}

void vtun_sig_init(void)
{
	interfaces = NULL;
	signal(SIGINT, vtun_sig_handle);
}

void vtun_sig_add_interface(ifr_name)
	const char *ifr_name;
{
	vtun_sig_interface_t *ifr;

	ifr = (vtun_sig_interface_t *)malloc(sizeof(*ifr));
	if (!ifr) {
		perror("malloc");
		exit(1);
	}

	ifr->next = interfaces;
	strcpy(ifr->ifr_name, ifr_name);

	interfaces = ifr;
}
