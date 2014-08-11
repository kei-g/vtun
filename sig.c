#include "sig.h"

#include "ioctl.h"
#include "vtun.h"

#include <signal.h>

typedef enum {
	VTUN_SIG_DEVICE,
	VTUN_SIG_NAME,
} vtun_sig_type_t;

typedef struct _vtun_sig_interface vtun_sig_interface_t;

struct _vtun_sig_interface {
	vtun_sig_interface_t *next;
	vtun_sig_type_t ifr_type;
	union {
		int ifr_device;
		char ifr_name[16];
	};
};

static vtun_sig_interface_t *interfaces = NULL;

static void vtun_sighup(sig)
	int sig;
{
	printf("HUP\n");
}

static void vtun_sigint(sig)
	int sig;
{
	printf("\n");
	exit(1);
}

static void vtun_sigterm(sig)
	int sig;
{
	exit(1);
}

static void vtun_sig_destroy_interfaces(void)
{
	vtun_sig_interface_t *ifr, *next;

	for (ifr = interfaces; ifr; ifr = next) {
		next = ifr->next;
		switch (ifr->ifr_type) {
		case VTUN_SIG_DEVICE:
			close(ifr->ifr_device);
			break;
		case VTUN_SIG_NAME:
			ioctl_destroy_interface(ifr->ifr_name);
			break;
		}
		free(ifr);
	}
}

void vtun_sig_init(void)
{
	interfaces = NULL;
	signal(SIGHUP, vtun_sighup);
	signal(SIGINT, vtun_sigint);
	signal(SIGTERM, vtun_sigterm);
}

void vtun_sig_add_interface_by_device(dev)
	int dev;
{
	vtun_sig_interface_t *ifr;

	ifr = (vtun_sig_interface_t *)malloc(sizeof(*ifr));
	if (!ifr) {
		perror("malloc");
		exit(1);
	}

	ifr->next = interfaces;
	ifr->ifr_type = VTUN_SIG_DEVICE;
	ifr->ifr_device = dev;

	interfaces = ifr;

	atexit(vtun_sig_destroy_interfaces);
}

void vtun_sig_add_interface_by_name(ifr_name)
	const char *ifr_name;
{
	vtun_sig_interface_t *ifr;

	ifr = (vtun_sig_interface_t *)malloc(sizeof(*ifr));
	if (!ifr) {
		perror("malloc");
		exit(1);
	}

	ifr->next = interfaces;
	ifr->ifr_type = VTUN_SIG_NAME;
	strncpy(ifr->ifr_name, ifr_name, sizeof(ifr->ifr_name));

	interfaces = ifr;

	atexit(vtun_sig_destroy_interfaces);
}
