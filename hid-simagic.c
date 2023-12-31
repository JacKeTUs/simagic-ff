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
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_WHEEL) },
	{ }
};
MODULE_DEVICE_TABLE(hid, simagic_devices);

static int simagic_probe(struct hid_device *hdev, 
				const struct hid_device_id *id) 
{
	int ret;
	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT & ~HID_CONNECT_FF);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err;
	}

	ret = hid_pidff_init_simagic(hdev);
	if (ret) {
		hid_warn(hdev, "Force feedback not supported\n");
		goto err;
	}

	return 0;
err:
	return ret;
}

static int simagic_input_configured(struct hid_device *hdev,
				struct hid_input *hidinput) 
{
	struct input_dev *input = hidinput->input;
	input_set_abs_params(input, ABS_X, 
		input->absinfo[ABS_X].minimum, input->absinfo[ABS_X].maximum, 0, 0);

	return 0;
}

static struct hid_driver simagic_ff = {
	.name = "simagic-ff",
	.id_table = simagic_devices,
	.probe = simagic_probe,
	.input_configured = simagic_input_configured,
};
module_hid_driver(simagic_ff);

MODULE_AUTHOR("Oleg Makarenko <oleg@makarenk.ooo>");
MODULE_DESCRIPTION("Simagic HID FF Driver");
MODULE_LICENSE("GPL");
