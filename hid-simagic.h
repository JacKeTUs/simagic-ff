/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __HID_SIMAGIC_H
#define __HID_SIMAGIC_H

#define USB_VENDOR_ID_SIMAGIC_ALPHA 0x0483
#define USB_VENDOR_ID_SIMAGIC       0x3670
#define USB_DEVICE_ID_SIMAGIC_ALPHA 0x0522
#define USB_DEVICE_ID_SIMAGIC_EVO   0x0500
#define USB_DEVICE_ID_SIMAGIC_EVO_1 0x0501
#define USB_DEVICE_ID_SIMAGIC_EVO_2 0x0502

#define	PID_EFFECTS_MAX		64

struct smff_device {
	struct hid_device *hid;
	int pid_id[PID_EFFECTS_MAX];

	bool is_alpha_evo;
	bool sysfs_created;
};

struct smff_device* get_smff_from_hid(struct hid_device *hid);
int sm_hid_get_report(struct hid_device *hid, u8 report, u8 *buffer, size_t len,
	enum hid_report_type rtype);
int sm_hid_set_report(struct hid_device *hid, u8 report, u8 *buffer, size_t len,
	enum hid_report_type rtype);

int hid_pidff_init_simagic(struct hid_device *hdev);

#endif
