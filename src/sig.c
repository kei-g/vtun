#include "sig.h"

#include "ioctl.h"
#include "vtun.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void vtun_sighup(int sig)
{
	printf("HUP\n");
}

static void vtun_sigint(int sig)
{
	printf("\n");
	exit(1);
}

static void vtun_sigterm(int sig)
{
	exit(1);
}

static void vtun_sig_destroy_interfaces(void)
{
	for (vtun_sig_interface_t *next, *ifr = interfaces; ifr; ifr = next) {
		next = ifr->next;
		switch (ifr->ifr_type) {
		case VTUN_SIG_DEVICE:
			close(ifr->ifr_device);
			break;
		case VTUN_SIG_NAME:
#if defined(__FreeBSD__)
			ioctl_destroy_interface(ifr->ifr_name);
#endif
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

void vtun_sig_add_interface_by_device(int dev)
{
	vtun_sig_interface_t *ifr = malloc(sizeof(*ifr));
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

void vtun_sig_add_interface_by_name(const char *name)
{
	vtun_sig_interface_t *ifr = malloc(sizeof(*ifr));
	if (!ifr) {
		perror("malloc");
		exit(1);
	}

	ifr->next = interfaces;
	ifr->ifr_type = VTUN_SIG_NAME;
	strncpy(ifr->ifr_name, name, sizeof(ifr->ifr_name));

	interfaces = ifr;

	atexit(vtun_sig_destroy_interfaces);
}
