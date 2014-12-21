#ifndef	ASTRIBANK_H
#define	ASTRIBANK_H

#include <mpptalk.h>

struct astribank *astribank_new(const char *path);
void astribank_destroy(struct astribank *ab);
void show_astribank_info(const struct astribank *ab);

struct xusb_iface *astribank_xpp_open(struct astribank *ab);
struct mpp_device *astribank_mpp_open(struct astribank *ab);

struct xusb_device *xusb_dev_of_astribank(const struct astribank *ab);
const char *astribank_devpath(const struct astribank *ab);
const char *astribank_serial(const struct astribank *ab);

int astribank_send(struct astribank *ab, int interface_num, const char *buf, int len, int timeout);
int astribank_recv(struct astribank *ab, int interface_num, char *buf, size_t len, int timeout);


#define AB_REPORT(report_type, astribank, fmt, ...) \
	report_type("%s [%s]: " fmt, \
		astribank_devpath(astribank), \
		astribank_serial(astribank), \
		## __VA_ARGS__)

#define AB_INFO(astribank, fmt, ...) \
		AB_REPORT(INFO, astribank, fmt, ## __VA_ARGS__)

#define AB_ERR(astribank, fmt, ...) \
		AB_REPORT(ERR, astribank, fmt, ## __VA_ARGS__)

#endif	/* ASTRIBANK_H */
