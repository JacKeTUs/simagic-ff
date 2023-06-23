// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HID driver for Simagic Steering Wheels
 *
 * Copyright (c) 2023 Makarenko Oleg
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include "hid-simagic.h"

static const struct hid_device_id simagic_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_ALPHA) },
	{ }
};
MODULE_DEVICE_TABLE(hid, simagic_devices);

static int simagic_probe(struct hid_device *hdev, const struct hid_device_id *id) {
	int ret;
	hid_info(hdev, "Probing simagic\n");
	hdev->ff_init = hid_pidff_init_simagic;
	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err;
	}

	return 0;
err:
	return ret;
}


static struct hid_driver simagic_ff = {
	.name = "simagic-ff",
	.id_table = simagic_devices,
	.probe = simagic_probe,
};
module_hid_driver(simagic_ff);

MODULE_AUTHOR("Oleg Makarenko <oleg@makarenk.ooo>");
MODULE_DESCRIPTION("Simagic HID FF Driver");
MODULE_LICENSE("GPL");
