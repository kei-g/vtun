#ifndef __include_sig_h__
#define __include_sig_h__

void vtun_sig_init(void);
void vtun_sig_add_interface_by_device(int dev);
void vtun_sig_add_interface_by_name(const char *ifr_name);

#endif /* __include_sig_h__ */
