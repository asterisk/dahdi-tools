#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include "mpptalk.h"
#include "astribank.h"

#define	DBG_MASK	0x08

struct astribank {
        struct xusb_device      *xusb_device;
	struct xusb_iface	*xpp_iface;
	struct xusb_iface	*mpp_iface;
        struct mpp_device       *mpp;
	char *path;
};


struct astribank *astribank_new(const char *path)
{
	struct astribank *ab;

	ab = calloc(sizeof(*ab), 1);
	if (!ab) {
		ERR("%s: Failed allocating Astribank device\n", path);
		goto err;
	}
	ab->xusb_device = xusb_find_bypath(path);
	if (!ab->xusb_device) {
		ERR("%s: Cannot find Astribank\n", path);
		goto err;
	}
	ab->path = strdup(path);
	if (!ab->path) {
		ERR("%s: Failed allocating Astribank path\n", path);
		goto err;
	}
	return ab;
err:
	astribank_destroy(ab);
	return NULL;
}

void astribank_destroy(struct astribank *ab)
{
	if (ab) {
		if (ab->path)
			free(ab->path);
		if (ab->xpp_iface)
			xusb_release(ab->xpp_iface);
		if (ab->mpp) {
			mpp_delete(ab->mpp); /* this also closes the underlying xusb */
			ab->mpp = NULL;
		}
		if (ab->xusb_device) {
			xusb_destroy(ab->xusb_device);
			ab->xusb_device = NULL;
		}
		free(ab);
		ab = NULL;
	}
}

struct xusb_iface *astribank_xpp_open(struct astribank *ab)
{
	int ret;

        ret = xusb_claim(ab->xusb_device, 0, &ab->xpp_iface);
        if (ret < 0) {
		ERR("%s: Cannot claim XPP interface\n", ab->path);
		goto err;
        }
	DBG("%s: Claimed Astribank XPP interface\n", ab->path);
	return ab->xpp_iface;
err:
	if (ab->xpp_iface)
		xusb_release(ab->xpp_iface);
	return NULL;
}

struct mpp_device *astribank_mpp_open(struct astribank *ab)
{
	int ret;

	ret = xusb_claim(ab->xusb_device, 1, &ab->mpp_iface);
	if (ret < 0) {
		ERR("%s: Cannot claim MPP interface\n", ab->path);
		goto err;
	}
	DBG("%s: Claimed Astribank MPP interface\n", ab->path);
	ab->mpp = mpp_new(ab->mpp_iface);
	if (!ab->mpp) {
		ERR("Failed initializing MPP protocol\n");
		goto err;
	}
	ret = mpp_status_query(ab->mpp);
	if (ret < 0) {
		ERR("status query failed (ret=%d)\n", ret);
		goto err;
	}
	return ab->mpp;
err:
	if (ab->mpp) {
		mpp_delete(ab->mpp); /* this also closes the underlying xusb */
		ab->mpp = NULL;
	}
	return NULL;
}

struct xusb_device *xusb_dev_of_astribank(const struct astribank *ab)
{
	assert(ab->xusb_device);
	return ab->xusb_device;
}

const char *astribank_devpath(const struct astribank *ab)
{
	return xusb_devpath(ab->xusb_device);
}

const char *astribank_serial(const struct astribank *ab)
{
	return xusb_serial(ab->xusb_device);
}

void show_astribank_info(const struct astribank *ab)
{
	struct xusb_device *xusb_device;

	assert(ab != NULL);
	xusb_device = ab->xusb_device;
	assert(xusb_device != NULL);
	if(verbose <= LOG_INFO) {
		xusb_showinfo(xusb_device);
	} else {
		const struct xusb_spec  *spec;

		spec = xusb_spec(xusb_device);
		printf("USB    Bus/Device:    [%s]\n", xusb_devpath(xusb_device));
		printf("USB    Firmware Type: [%s]\n", spec->name);
		printf("USB    iSerialNumber: [%s]\n", xusb_serial(xusb_device));
		printf("USB    iManufacturer: [%s]\n", xusb_manufacturer(xusb_device));
		printf("USB    iProduct:      [%s]\n", xusb_product(xusb_device));
	}
}

int astribank_send(struct astribank *ab, int interface_num, const char *buf, int len, int timeout)
{
	struct xusb_iface *iface;

	if (interface_num == 0)
		iface = ab->xpp_iface;
	else if (interface_num == 1)
		iface = ab->mpp_iface;
	else {
		ERR("Unknown interface number (%d)\n", interface_num);
		return -EINVAL;
	}
	return xusb_send(iface, buf, len, timeout);
}

int astribank_recv(struct astribank *ab, int interface_num, char *buf, size_t len, int timeout)
{
	struct xusb_iface *iface;

	if (interface_num == 0)
		iface = ab->xpp_iface;
	else if (interface_num == 1)
		iface = ab->mpp_iface;
	else {
		ERR("Unknown interface number (%d)\n", interface_num);
		return -EINVAL;
	}
	return xusb_recv(iface, buf, len, timeout);
}
