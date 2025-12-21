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

struct smff_status1_report {
	u8 report_id;
	u8 unknown_offset_01;
	__le16 max_angle;
	u8 ff_strength_percentage;
	u8 unknown_offset_05[2];
	u8 wheel_rotation_speed;
	u8 unknown_offset_08[3];
	u8 mechanical_friction_percentage;
	u8 game_centering;
	u8 game_inertia;
	u8 game_damper;
	u8 game_friction;
	__le16 angle_lock;
	u8 feedback_detail;
	u8 unknown_offset_19;
	u8 angle_lock_strength; // 0 = Soft, 1 = Normal, 2 = Firm 
	u8 unknown_offset_21[29];
	u8 filter_level;
	u8 unknown_offset_51;
	u8 slew_rate_control;
	u8 unknown_offset_53[2];
	u8 wheel_channel;
	u8 unknown_offset_55; // voltage?
	u8 unknown_offset_56[7];
};

struct smff_device {
	struct hid_device *hid;
	int pid_id[PID_EFFECTS_MAX];

	bool sysfs_created;
};

struct smff_device* get_smff_from_hid(struct hid_device *hid);
bool sm_read_status1(struct hid_device *hid, struct smff_status1_report *out_status);

int hid_pidff_init_simagic(struct hid_device *hdev);

#endif
