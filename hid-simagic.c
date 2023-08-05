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

#define	PID_EFFECTS_MAX		64

#define SM_SET_EFFECT_REPORT 0x01
#define SM_SET_ENVELOPE_REPORT 0x12

#define SM_SET_CONDITION_REPORT 0x03
#define SM_SET_PERIODIC_REPORT 0x04
#define SM_SET_CONSTANT_REPORT 0x05
#define SM_SET_RAMP_FORCE_REPORT 0x16
#define SM_SET_CUSTOM_FORCE_REPORT 0x17

#define SM_EFFECT_OPERATION_REPORT 0x0a

#define SM_CONSTANT 0x01
#define SM_DAMPER 0x05
#define SM_SPRING 0x06


struct smff_device {
	struct hid_device *hid;
	int pid_id[PID_EFFECTS_MAX];


}

static const struct hid_device_id simagic_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SIMAGIC, USB_DEVICE_ID_SIMAGIC_ALPHA) },
	{ }
};


MODULE_DEVICE_TABLE(hid, simagic_devices);


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

static int sm_set_constant_report(struct input_dev *dev, struct ff_effect *effect)
{
	int i;

	struct hid_device *hid = input_get_drvdata(dev);
	hid_info(dev, "Constant upload: type %u, id: %u\n", effect->type, effect->id);

	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	s32 *value = report->field[0]->value;
	hid_info(dev, "report id: %d", report->id);

	for (int i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_SET_CONSTANT_REPORT;
	switch (effect->type)
	{
	case FF_CONSTANT:
		value[1] = SM_CONSTANT;
		break;
	default:
		break;
	}

	hid_info(dev, "Const params: level: %d\n", 
		effect->u.constant.level);

	int mag = sm_rescale_signed_to_10k(effect->u.constant.level);//, 0x8000, 10000, 65535);
	hid_info(dev, "Const params: level scaled: %d\n", 
		mag);
	value[2] = mag & 0x00ff;
	value[3] = (mag & 0xff00) >> 8;

	hid_hw_request(hid, report, HID_REQ_SET_REPORT);
	return 0;
}

static int sm_set_condition_report(struct input_dev *dev, struct ff_effect *effect)
{
	int i;

	struct hid_device *hid = input_get_drvdata(dev);
	hid_info(dev, "Condition upload: type %u, id: %u\n", effect->type, effect->id);

	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	s32 *value = report->field[0]->value;
	hid_info(dev, "report id: %d", report->id);

	for (int i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_SET_CONDITION_REPORT; // upd cond
	switch (effect->type)
	{
	case FF_SPRING:
		value[1] = SM_SPRING;
		break;
	case FF_DAMPER:
		value[1] = SM_DAMPER;
		break;
	default:
		break;
	}

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
	hid_info(dev, "NEW Effect upload: ef type: %d\n", effect->type);
	struct hid_device *hid = input_get_drvdata(dev);
	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	s32 *value = report->field[0]->value;
	
	for (int i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_SET_EFFECT_REPORT;
	switch (effect->type)
	{
	case FF_CONSTANT:
		value[1] = SM_CONSTANT;
		break;
	case FF_SPRING:
		value[1] = SM_SPRING;
		break;
	case FF_DAMPER:
		value[1] = SM_DAMPER;
		break;	
	default:
		break;
	}
	
	int dur = effect->replay.length == 0 ? 0xffff: effect->replay.length;
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
	hid_info(dev, "Effect upload: type %u, id: %u\n", effect->type, effect->id);

	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	s32 *value = report->field[0]->value;
	hid_info(dev, "report id: %d", report->id);
	int i;
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
	struct hid_device *hid = input_get_drvdata(dev);
	struct smff_device *pidff = dev->ff->private;
	hid_info(dev, "Effect erase: id: %u\n", effect_id);
	return 0;
}



static int sm_req_playback(struct input_dev *dev, int effect_id, int count) {
	struct hid_device *hid = input_get_drvdata(dev);
	//struct smff_device *pidff = dev->ff->private;
	hid_info(dev, "Effect play: id: %u, count %d\n", effect_id, count);	

	struct list_head *report_list = &hid->report_enum[HID_OUTPUT_REPORT].report_list;
	struct hid_report *report = list_entry(report_list->next, struct hid_report, list);

	s32 *value = report->field[0]->value;
	hid_info(dev, "report id: %d", report->id);
	int i;

	for (i = 0; i < 64; i++) {
		value[i] = 0;
	}
	value[0] = SM_EFFECT_OPERATION_REPORT;
	value[1] = effect_id; 
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

	// update all effects
	hid_info(dev, "PLAYBACK: ID: %d, count %d\n", effect_id, count);
	//sm_req_playback(dev, SM_SPRING, count);
	//sm_req_playback(dev, SM_DAMPER, count);
	sm_req_playback(dev, SM_CONSTANT, count);

	return 0;
}

static void sm_set_gain(struct input_dev *dev, u16 gain) {
}

static void sm_set_autocenter(struct input_dev *dev, u16 magnitude) {
}


static int simagic_ff_initffb(struct hid_device *hid) {
	struct hid_input *hidinput = list_entry(hid->inputs.next,
						struct hid_input, list);
	struct input_dev *dev = hidinput->input;
	struct ff_device *ff;
	int max_effects = 16;
	int error = -ENODEV;


	set_bit(FF_CONSTANT, dev->ffbit);
	set_bit(FF_RAMP, dev->ffbit);
	set_bit(FF_SQUARE, dev->ffbit);
	set_bit(FF_SINE, dev->ffbit);
	set_bit(FF_TRIANGLE, dev->ffbit);
	set_bit(FF_SAW_UP, dev->ffbit);
	set_bit(FF_SAW_DOWN, dev->ffbit);
	set_bit(FF_SPRING, dev->ffbit);
	set_bit(FF_DAMPER, dev->ffbit);
	set_bit(FF_INERTIA, dev->ffbit);
	set_bit(FF_FRICTION, dev->ffbit);
	set_bit(FF_PERIODIC, dev->ffbit);
	set_bit(FF_CUSTOM, dev->ffbit);

	error = input_ff_create(dev, max_effects);
	if (error)
		goto fail;
	
	ff = dev->ff;
	ff->upload = sm_upload;
	ff->erase = sm_erase;
	ff->playback = sm_playback;
	ff->set_gain = sm_set_gain;
	ff->set_autocenter = sm_set_autocenter;


	hid_info(dev, "Force feedback for Simagic wheel\n");
	return 0;
  fail:
	return error;
}

static int simagic_probe(struct hid_device *hdev, const struct hid_device_id *id) {
	int ret;
	//hdev->ff_init = hid_pidff_init_simagic;
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

static int simagic_input_configured(struct hid_device *hdev,
				struct hid_input *hidinput) {
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
