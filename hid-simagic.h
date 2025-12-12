/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __HID_SIMAGIC_H
#define __HID_SIMAGIC_H

#define USB_VENDOR_ID_SIMAGIC_ALPHA 0x0483
#define USB_VENDOR_ID_SIMAGIC       0x3670
#define USB_DEVICE_ID_SIMAGIC_ALPHA 0x0522
#define USB_DEVICE_ID_SIMAGIC_EVO   0x0500
#define USB_DEVICE_ID_SIMAGIC_EVO_1 0x0501
#define USB_DEVICE_ID_SIMAGIC_EVO_2 0x0502

int hid_pidff_init_simagic(struct hid_device *hdev);

#endif
