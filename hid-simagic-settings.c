#include <linux/device.h>
#include <linux/hid.h>
#include "hid-simagic-settings.h"
#include "hid-simagic.h"

#define SM_SET_WHEEL_SETTINGS1_REPORT 0x80
#define SM_GET_STATUS1_REPORT 0x81

bool sm_read_status1(struct hid_device *hid, struct smff_status1_report *out_status) {
	int ret;

	if (!out_status)
		return false;
	
	ret = sm_hid_get_report(hid, SM_GET_STATUS1_REPORT, (u8*)out_status,
				sizeof(*out_status), HID_FEATURE_REPORT);

	if (ret < 0) {
		hid_err(hid, "Failed to retrieve wheel settings: %d\n", ret);
		return false;
	}

	return true;
}

bool sm_read_settings1(struct hid_device *hid, struct smff_settings1_report *out_settings)
{
	struct smff_status1_report status1;
	if (!out_settings || !sm_read_status1(hid, &status1))
		return false;
	
	out_settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	out_settings->unknown_offset_01    = 0x01;
	out_settings->max_angle            = status1.max_angle;
	out_settings->ff_strength          = status1.ff_strength;
	out_settings->unknown_offset_06    = status1.unknown_offset_06;
	out_settings->wheel_rotation_speed = status1.wheel_rotation_speed;
	out_settings->mechanical_centering = status1.mechanical_centering;
	out_settings->mechanical_damper    = status1.mechanical_damper;
	out_settings->center_damper        = status1.center_damper;
	out_settings->mechanical_friction  = status1.mechanical_friction;
	out_settings->game_centering       = status1.game_centering;
	out_settings->game_inertia         = status1.game_inertia;
	out_settings->game_damper          = status1.game_damper;
	out_settings->game_friction        = status1.game_friction;

	return true;
}

static void sm_sanitize_settings1_report(struct smff_settings1_report* settings) {
	if (!settings)
		return;
	
	settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	settings->unknown_offset_01    = 0x01;
	settings->max_angle            = cpu_to_le16(clamp_t(u16, le16_to_cpu(settings->max_angle), 90, 2520));
	settings->ff_strength          = cpu_to_le16((u16)clamp_t(s16, (s16)le16_to_cpu(settings->ff_strength), -100, 100));
	settings->unknown_offset_06    = 0x02;
	settings->wheel_rotation_speed = min_t(u8, settings->wheel_rotation_speed, 100);
	settings->mechanical_centering = min_t(u8, settings->mechanical_centering, 100);
	settings->mechanical_damper    = min_t(u8, settings->mechanical_damper, 100);
	settings->center_damper        = min_t(u8, settings->center_damper, 100);
	settings->mechanical_friction  = min_t(u8, settings->mechanical_friction, 100);
	settings->game_centering       = min_t(u8, settings->game_centering, 200);
	settings->game_inertia         = min_t(u8, settings->game_inertia, 200);
	settings->game_damper          = min_t(u8, settings->game_damper, 200);
	settings->game_friction        = min_t(u8, settings->game_friction, 200);
}

bool sm_write_settings1(struct hid_device *hid, struct smff_settings1_report *in_settings)
{
	struct smff_device *smff = get_smff_from_hid(hid);
	u8 raw_buffer[64];
	struct smff_settings1_report* report_buffer = (struct smff_settings1_report*)raw_buffer;
	int ret;

	static_assert(sizeof(*in_settings) <= sizeof(raw_buffer));

	if (!smff || !in_settings )
		return false;
	
	memset(raw_buffer, 0, sizeof(raw_buffer));
	memcpy(raw_buffer, in_settings, sizeof(*in_settings));

	sm_sanitize_settings1_report(report_buffer);

	ret = sm_hid_set_report(hid, SM_SET_WHEEL_SETTINGS1_REPORT, raw_buffer,
		sizeof(raw_buffer), HID_FEATURE_REPORT);
	
	if (ret < 0) {
		hid_err(hid, "Failed to set wheel settings: %d\n", ret);
		return false;
	}

	return true;
}

bool sm_read_settings2(struct hid_device *hid, struct smff_settings2_report *out_settings)
{
	struct smff_status1_report status1;
	if (!out_settings || !sm_read_status1(hid, &status1))
		return false;
	
	out_settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	out_settings->unknown_offset_01    = 0x02;
	out_settings->angle_lock           = status1.angle_lock;
	out_settings->feedback_detail      = status1.feedback_detail;
	out_settings->unknown_offset_06    = status1.unknown_offset_19;
	out_settings->angle_lock_strength  = status1.angle_lock_strength;
	out_settings->unknown_offset_08    = status1.unknown_offset_21;
	out_settings->mechanical_inertia   = status1.mechanical_inertia;
	out_settings->unknown_offset_10    = status1.unknown_offset_23;

	return true;
}

static void sm_sanitize_settings2_report(struct hid_device *hid, struct smff_settings2_report* settings) {
	struct smff_status1_report status1;

	if (!settings)
		return;

	s16 max_angle = 2520;
	if (sm_read_status1(hid, &status1)) {
		max_angle = le16_to_cpu(status1.max_angle);
		settings->unknown_offset_06 = status1.unknown_offset_19;
		settings->unknown_offset_08 = status1.unknown_offset_21;
		settings->unknown_offset_10 = status1.unknown_offset_23;
	}
	
	settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	settings->unknown_offset_01    = 0x02;
	settings->angle_lock           = cpu_to_le16(clamp_t(u16, le16_to_cpu(settings->angle_lock), 90, max_angle));
	settings->feedback_detail      = min_t(u8, settings->feedback_detail, 100);
	settings->angle_lock_strength  = min_t(u8, settings->angle_lock_strength, 2);
	settings->mechanical_inertia   = min_t(u8, settings->mechanical_inertia, 100);
}

bool sm_write_settings2(struct hid_device *hid, struct smff_settings2_report *in_settings)
{
	struct smff_device *smff = get_smff_from_hid(hid);
	u8 raw_buffer[64];
	struct smff_settings2_report* report_buffer = (struct smff_settings2_report*)raw_buffer;
	int ret;

	static_assert(sizeof(*in_settings) <= sizeof(raw_buffer));

	if (!smff || !in_settings )
		return false;
	
	memset(raw_buffer, 0, sizeof(raw_buffer));
	memcpy(raw_buffer, in_settings, sizeof(*in_settings));

	sm_sanitize_settings2_report(hid, report_buffer);

	ret = sm_hid_set_report(hid, SM_SET_WHEEL_SETTINGS1_REPORT, raw_buffer,
		sizeof(raw_buffer), HID_FEATURE_REPORT);
	
	if (ret < 0) {
		hid_err(hid, "Failed to set wheel settings: %d\n", ret);
		return false;
	}

	return true;
}

bool sm_read_settings3(struct hid_device *hid, struct smff_settings3_report *out_settings)
{
	struct smff_status1_report status1;
	if (!out_settings || !sm_read_status1(hid, &status1))
		return false;
	
	out_settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	out_settings->unknown_offset_01    = 0x10;
	out_settings->unknown_offset_02    = 0x38;
	out_settings->unknown_offset_03    = 0x00;
	out_settings->unknown_offset_04    = 0x01;
	out_settings->ring_light           = status1.ring_light;

	return true;
}

static void sm_sanitize_settings3_report(struct hid_device *hid, struct smff_settings3_report* settings) {
	u8 ring_light_enabled;
	u8 ring_light_brightness;

	if (!settings)
		return;

	ring_light_enabled = settings->ring_light & 0x80;
	ring_light_brightness = clamp_t(u8, settings->ring_light & 0x7f, 0, 100);
	
	settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	settings->unknown_offset_01    = 0x10;
	settings->unknown_offset_02    = 0x38;
	settings->unknown_offset_03    = 0x00;
	settings->unknown_offset_04    = 0x01;
	settings->ring_light           = ring_light_enabled | ring_light_brightness;
}

bool sm_write_settings3(struct hid_device *hid, struct smff_settings3_report *in_settings)
{
	struct smff_device *smff = get_smff_from_hid(hid);
	u8 raw_buffer[64];
	struct smff_settings3_report* report_buffer = (struct smff_settings3_report*)raw_buffer;
	int ret;

	static_assert(sizeof(*in_settings) <= sizeof(raw_buffer));

	if (!smff || !in_settings )
		return false;
	
	memset(raw_buffer, 0, sizeof(raw_buffer));
	memcpy(raw_buffer, in_settings, sizeof(*in_settings));

	sm_sanitize_settings3_report(hid, report_buffer);

	ret = sm_hid_set_report(hid, SM_SET_WHEEL_SETTINGS1_REPORT, raw_buffer,
		sizeof(raw_buffer), HID_FEATURE_REPORT);
	
	if (ret < 0) {
		hid_err(hid, "Failed to set wheel settings: %d\n", ret);
		return false;
	}

	return true;
}

bool sm_read_settings4(struct hid_device *hid, struct smff_settings4_report *out_settings)
{
	struct smff_status1_report status1;
	if (!out_settings || !sm_read_status1(hid, &status1))
		return false;
	
	out_settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	out_settings->unknown_offset_01    = 0x10;
	out_settings->unknown_offset_02    = 0x39;
	out_settings->unknown_offset_03    = 0x00;
	out_settings->unknown_offset_04    = 0x07;
	out_settings->unknown_offset_05    = status1.unknown_offset_48;
	out_settings->unknown_offset_06    = status1.unknown_offset_49;
	out_settings->filter_level         = status1.filter_level;
	out_settings->unknown_offset_08    = status1.unknown_offset_51;
	out_settings->slew_rate_control    = status1.slew_rate_control;

	return true;
}

static void sm_sanitize_settings4_report(struct hid_device *hid, struct smff_settings4_report* settings) {
	struct smff_status1_report status1;
	if (!settings)
		return;

	if (sm_read_status1(hid, &status1)) {
		settings->unknown_offset_05 = status1.unknown_offset_48;
		settings->unknown_offset_06 = status1.unknown_offset_49;
		settings->unknown_offset_08 = status1.unknown_offset_51;
	}
	
	settings->report_id            = SM_SET_WHEEL_SETTINGS1_REPORT;
	settings->unknown_offset_01    = 0x10;
	settings->unknown_offset_02    = 0x39;
	settings->unknown_offset_03    = 0x00;
	settings->unknown_offset_04    = 0x07;
	settings->filter_level         = clamp_t(u8, settings->filter_level, 0, 20);
	settings->slew_rate_control    = clamp_t(u8, settings->slew_rate_control, 0, 100);
}

bool sm_write_settings4(struct hid_device *hid, struct smff_settings4_report *in_settings)
{
	struct smff_device *smff = get_smff_from_hid(hid);
	u8 raw_buffer[64];
	struct smff_settings4_report* report_buffer = (struct smff_settings4_report*)raw_buffer;
	int ret;

	static_assert(sizeof(*in_settings) <= sizeof(raw_buffer));

	if (!smff || !in_settings )
		return false;
	
	memset(raw_buffer, 0, sizeof(raw_buffer));
	memcpy(raw_buffer, in_settings, sizeof(*in_settings));

	sm_sanitize_settings4_report(hid, report_buffer);

	ret = sm_hid_set_report(hid, SM_SET_WHEEL_SETTINGS1_REPORT, raw_buffer,
		sizeof(raw_buffer), HID_FEATURE_REPORT);
	
	if (ret < 0) {
		hid_err(hid, "Failed to set wheel settings: %d\n", ret);
		return false;
	}

	return true;
}