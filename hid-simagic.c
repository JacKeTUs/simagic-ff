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
#include "hid-simagic-sysfs.h"

#define SM_HID_MAX_TMPBUF_SIZE 64

#define SM_SET_EFFECT_REPORT 0x01
#define SM_SET_ENVELOPE_REPORT 0x12

#define SM_SET_CONDITION_REPORT 0x03
#define SM_SET_PERIODIC_REPORT 0x04
#define SM_SET_CONSTANT_REPORT 0x05
#define SM_SET_RAMP_FORCE_REPORT 0x16
#define SM_SET_CUSTOM_FORCE_REPORT 0x17

#define SM_EFFECT_OPERATION_REPORT 0x0a

#define SM_SET_WHEEL_SETTINGS1_REPORT 0x80
#define SM_GET_STATUS1_REPORT 0x81

#define SM_CONSTANT 0x01
#define SM_DAMPER 0x05
#define SM_SPRING 0x06

static const struct hid_device_id simagic_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC_ALPHA, USB_DEVICE_ID_SIMAGIC_ALPHA) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_EVO) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_EVO_1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_EVO_2) },
	{ }
};


MODULE_DEVICE_TABLE(hid, simagic_devices);

struct smff_device* get_smff_from_hid(struct hid_device *hid) {
	struct hid_input *hidinput = list_entry(hid->inputs.next,
						struct hid_input, list);
	struct input_dev *dev;
	struct ff_device *ff;

	if (hidinput == NULL)
		return NULL;
	
	dev = hidinput->input;
	if (dev == NULL)
		return NULL;
	
	ff = dev->ff;
	if (ff == NULL)
		return NULL;

	return ff->private;
}

/*
 * Scale an unsigned value with range 0..max for the given field
 */
static int sm_rescale_coeffs(int i, int max, int min_field, int max_field)
{
	return i * (max_field - min_field) / max + min_field;
}

static int sm_rescale_signed_to_10k(int i)
{
	return i == 0 ? 0 : i >
	    0 ? i * 10000 / 0x7fff : i *
	    -10000 / -0x8000;
}

static int get_block_id(struct ff_effect *effect) {
	int sm_pid_id = 0;
	switch (effect->type)
	{
	case FF_CONSTANT:
		sm_pid_id = SM_CONSTANT;
		break;
	case FF_DAMPER:
		sm_pid_id = SM_DAMPER;
		break;
	case FF_SPRING:
		sm_pid_id = SM_SPRING;
		break;
	default:
		break;
	}
	return sm_pid_id;
}

static int sm_hid_get_report(struct hid_device *hid, u8 report, u8 *buffer, size_t len,
	enum hid_report_type rtype) {

	struct smff_device *smff;
	int ret;
	u8* tmp_buf;

	if (!hid || len > SM_HID_MAX_TMPBUF_SIZE)
		return -EINVAL;
	
	smff = get_smff_from_hid(hid);

	if (!smff)
		return -EINVAL;

	tmp_buf = kzalloc(len, GFP_KERNEL);
	ret = hid_hw_raw_request(hid, report, tmp_buf, len, rtype, HID_REQ_GET_REPORT);
	if (ret >= 0) {
		hid_hw_wait(hid);
		memcpy(buffer, tmp_buf, len);
	}
	kfree(tmp_buf);

	return ret;
}

static int sm_hid_set_report(struct hid_device *hid, u8 report, u8 *buffer, size_t len,
	enum hid_report_type rtype) {

	struct smff_device *smff;
	int ret;
	u8* tmp_buf;

	if (!hid || len > SM_HID_MAX_TMPBUF_SIZE)
		return -EINVAL;
	
	smff = get_smff_from_hid(hid);

	if (!smff)
		return -EINVAL;

	tmp_buf = kzalloc(len, GFP_KERNEL);
	memcpy(tmp_buf, buffer, len);
	ret = hid_hw_raw_request(hid, report, tmp_buf, len, rtype, HID_REQ_SET_REPORT);
	if (ret >= 0) {
		hid_hw_wait(hid);
	}
	kfree(tmp_buf);

	return ret;
}

static int sm_set_constant_report(struct input_dev *dev, struct ff_effect *effect)
{
	struct hid_device *hid = input_get_drvdata(dev);
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);
	s32 *value = report->field[0]->value;

	hid_info(dev, "Constant upload: type %u, id: %u\n", effect->type, effect->id);
	hid_info(dev, "report id: %d", report->id);

	for (int i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_SET_CONSTANT_REPORT;
	value[1] = get_block_id(effect);
	hid_info(dev, "Const params: level: %d\n", 
		effect->u.constant.level);

	int mag = sm_rescale_signed_to_10k(effect->u.constant.level);
	hid_info(dev, "Const params: level scaled: %d\n", 
		mag);
	value[2] = mag & 0x00ff;
	value[3] = (mag & 0xff00) >> 8;

	hid_hw_request(hid, report, HID_REQ_SET_REPORT);
	return 0;
}

static int sm_set_condition_report(struct input_dev *dev, struct ff_effect *effect)
{
	struct hid_device *hid = input_get_drvdata(dev);
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);
	s32 *value = report->field[0]->value;

	hid_info(dev, "Condition upload: type %u, id: %u\n", effect->type, effect->id);
	hid_info(dev, "report id: %d", report->id);

	for (int i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_SET_CONDITION_REPORT; // upd cond
	value[1] = get_block_id(effect);

	hid_info(dev, "Condition[0] params: center %d, rightC %d, leftC %d, rightS %d, leftS %d, deadband %d\n", 
		effect->u.condition[0].center, effect->u.condition[0].right_coeff, effect->u.condition[0].left_coeff, 
		effect->u.condition[0].right_saturation, effect->u.condition[0].left_saturation, 
		effect->u.condition[0].deadband);
	hid_info(dev, "Condition[1] params: center %d, rightC %d, leftC %d, rightS %d, leftS %d, deadband %d\n", 
		effect->u.condition[1].center, effect->u.condition[1].right_coeff, effect->u.condition[1].left_coeff, 
		effect->u.condition[1].right_saturation, effect->u.condition[1].left_saturation, 
		effect->u.condition[1].deadband);

	int center = sm_rescale_signed_to_10k(effect->u.condition[0].center);
	int right_coeff = sm_rescale_signed_to_10k(effect->u.condition[0].right_coeff);
	int left_coeff = sm_rescale_signed_to_10k(effect->u.condition[0].left_coeff);
	int right_sat = sm_rescale_coeffs(effect->u.condition[0].right_saturation, 0xffff, -10000, 10000);
	int left_sat = sm_rescale_coeffs(effect->u.condition[0].left_saturation, 0xffff, -10000, 10000);
	int deadband = sm_rescale_coeffs(effect->u.condition[0].deadband, 0xffff, 0,10000);

	hid_info(dev, "Condition[0] params scaled: center %d, rightC %d, leftC %d, rightS %d, leftS %d, deadband %d\n", 
		center, right_coeff, left_coeff, 
		right_sat, left_sat, 
		deadband);


	value[2] = 0x00;	
	value[3] = center & 0x00ff;
	value[4] = (center & 0xff00) >> 8;
	value[5] = right_coeff & 0x00ff;
	value[6] = (right_coeff & 0xff00) >> 8;
	value[7] = left_coeff & 0x00ff;
	value[8] = (left_coeff & 0xff00) >> 8;
	value[9] = right_sat & 0x00ff;
	value[10] = (right_sat & 0xff00) >> 8;
	value[11] = left_sat & 0x00ff;
	value[12] =(left_sat & 0xff00) >> 8;
	value[13] = deadband & 0x00ff;
	value[14] = (deadband  & 0xff00) >> 8;

	hid_hw_request(hid, report, HID_REQ_SET_REPORT);
	return 0;
}

static int sm_set_effect_report(struct input_dev *dev, struct ff_effect *effect) {
	struct hid_device *hid = input_get_drvdata(dev);
	struct smff_device *smff = dev->ff->private;
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);
	s32 *value = report->field[0]->value;
	int dur = effect->replay.length == 0 ? 0xffff: effect->replay.length;
	int i;

	hid_info(dev, "NEW Effect upload: ef type: %d\n", effect->type);
	for (i = 0; i < 64; i++) {
		value[i] = 0;
	}
	smff->pid_id[effect->id] = get_block_id(effect);
	value[0] = SM_SET_EFFECT_REPORT;
	value[1] = get_block_id(effect);
	value[2] = 0x01; // Always 1??? Multiple effects of one type simultaniously??
	value[3] = dur & 0x00ff; 		// Duration
	value[4] = (dur & 0xff00) >> 8; // --
	value[9] = 0xff; // Gain
	value[10] = 0xff; // Trigger Button
	value[11] = 0x04;
	value[12] = 0x3f;

	hid_hw_request(hid, report, HID_REQ_SET_REPORT);
	return 0;
}

static int sm_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old) {
	struct hid_device *hid = input_get_drvdata(dev);
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	hid_info(dev, "Effect upload: type %u, id: %u\n", effect->type, effect->id);
	hid_info(dev, "report id: %d", report->id);
	if (!old) {
		sm_set_effect_report(dev, effect);
	}
	if (effect->type == FF_CONSTANT) {
		sm_set_constant_report(dev, effect);
	}
	else {
		sm_set_condition_report(dev, effect);
	}

	return 0;
}

static int sm_erase(struct input_dev *dev, int effect_id) {
	//struct hid_device *hid = input_get_drvdata(dev);
	struct smff_device *smff = dev->ff->private;
	hid_info(dev, "Effect erase: id: %d/%d\n", effect_id, smff->pid_id[effect_id]);
	return 0;
}



static int sm_req_playback(struct input_dev *dev, int sm_block_id, int count) {
	struct hid_device *hid = input_get_drvdata(dev);
	int i;
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);
	s32 *value = report->field[0]->value;

	hid_info(dev, "Effect play: sm_id: %u, count %d\n", sm_block_id, count);	
	hid_info(dev, "report id: %d", report->id);

	for (i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_EFFECT_OPERATION_REPORT;
	value[1] = sm_block_id; 
	if (count > 0) {
		value[2] = 0x01; // 1 - OpEffectStart
		value[3] = count > 0xff ? 0xff : count; // loop count
	}
	else {
		value[2] = 0x03; // 3 - OpEffectStop
		value[3] = 0x00; // loop count
	}
	value[4] = 0x00;
	hid_hw_request(hid, report, HID_REQ_SET_REPORT);
	return 0;
}

static int sm_playback(struct input_dev *dev, int effect_id, int count) {
	struct smff_device *pidff = dev->ff->private;
	hid_info(dev, "PLAYBACK: ID: %d, count %d\n", effect_id, count);
	sm_req_playback(dev, pidff->pid_id[effect_id], count);
	return 0;
}

static void sm_set_gain(struct input_dev *dev, u16 gain) {
	hid_info(dev, "Setting gain: %d\n", gain);
}

static void sm_set_autocenter(struct input_dev *dev, u16 magnitude) {
	hid_info(dev, "Setting autocenter: %d\n", magnitude);
}

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
	settings->max_angle            = cpu_to_le16((u16)clamp_t(s16, (s16)le16_to_cpu(settings->max_angle), 90, 2520));
	settings->ff_strength          = cpu_to_le16((u16)clamp_t(s16, (s16)le16_to_cpu(settings->ff_strength), -100, 100));
	settings->unknown_offset_06    = 0x02;
	settings->wheel_rotation_speed = min_t(u8, settings->wheel_rotation_speed, 100);
	settings->mechanical_centering = min_t(u8, settings->mechanical_centering, 100);
	settings->mechanical_damper    = min_t(u8, settings->mechanical_damper, 100);
	settings->center_damper        = min_t(u8, settings->center_damper, 100);
	settings->mechanical_friction  = min_t(u8, settings->mechanical_friction, 100);
	settings->game_centering       = min_t(u8, settings->game_centering, 100);
	settings->game_inertia         = min_t(u8, settings->game_inertia, 100);
	settings->game_damper          = min_t(u8, settings->game_damper, 100);
	settings->game_friction        = min_t(u8, settings->game_friction, 100);
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

static int simagic_ff_initffb(struct hid_device *hid) {
	struct hid_input *hidinput = list_entry(hid->inputs.next,
						struct hid_input, list);
	struct input_dev *dev = hidinput->input;
	struct ff_device *ff;
	struct smff_device *smff;
	int max_effects = 16;
	int error = -ENODEV;

	smff = kzalloc(sizeof(*smff), GFP_KERNEL);
	if (!smff)
		return -ENOMEM;
	smff->hid = hid;
	
	set_bit(FF_CONSTANT, dev->ffbit);
	/*set_bit(FF_RAMP, dev->ffbit);
	set_bit(FF_SQUARE, dev->ffbit);
	set_bit(FF_SINE, dev->ffbit);
	set_bit(FF_TRIANGLE, dev->ffbit);
	set_bit(FF_SAW_UP, dev->ffbit);
	set_bit(FF_SAW_DOWN, dev->ffbit);*/
	set_bit(FF_SPRING, dev->ffbit);
	set_bit(FF_DAMPER, dev->ffbit);
	/*set_bit(FF_INERTIA, dev->ffbit);
	set_bit(FF_FRICTION, dev->ffbit);
	set_bit(FF_PERIODIC, dev->ffbit);
	set_bit(FF_CUSTOM, dev->ffbit);*/

	error = input_ff_create(dev, max_effects);
	if (error)
		goto fail;
	
	ff = dev->ff;
	ff->private = smff;
	ff->upload = sm_upload;
	ff->erase = sm_erase;
	ff->playback = sm_playback;
	ff->set_gain = sm_set_gain;
	ff->set_autocenter = sm_set_autocenter;

	simagic_ff_initsysfs(hid);

	hid_info(dev, "Force feedback for Simagic wheel\n");

	return 0;
  fail:
	kfree(smff);
	return error;
}

static int simagic_probe(struct hid_device *hdev, const struct hid_device_id *id) {
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

	ret = simagic_ff_initffb(hdev);
	if (ret) {
		hid_warn(hdev, "No force feedback\n");
		goto err;
	}
	return 0;
err:
	return ret;
}

static void simagic_remove(struct hid_device *hdev) {
	simagic_ff_removesysfs(hdev);
	hid_hw_stop(hdev);
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
	.remove = simagic_remove,
	.input_configured = simagic_input_configured,
};
module_hid_driver(simagic_ff);

MODULE_AUTHOR("Oleg Makarenko <oleg@makarenk.ooo>");
MODULE_DESCRIPTION("Simagic HID FF Driver");
MODULE_LICENSE("GPL");
