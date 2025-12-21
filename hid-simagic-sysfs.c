#include <linux/device.h>
#include <linux/hid.h>
#include "hid-simagic-sysfs.h"
#include "hid-simagic.h"

#define SM_SYSFS_STATUS1_ATTR_RO_U8(name, struct_name) \
static ssize_t name##_show(struct device *dev, \
	struct device_attribute *attr, \
	char *buf) \
{ \
	struct hid_device *hid = to_hid_device(dev); \
	struct smff_status1_report status1; \
	u8 value; \
	if (!sm_read_status1(hid, &status1)) {\
		return sysfs_emit(buf, "-err-\n"); \
	} \
	value = (status1).struct_name; \
	return sysfs_emit(buf, "%u\n", value); \
} \
static DEVICE_ATTR_RO(name);

#define SM_SYSFS_STATUS1_ATTR_RO_U16(name, struct_name) \
static ssize_t name##_show(struct device *dev, \
	struct device_attribute *attr, \
	char *buf) \
{ \
	struct hid_device *hid = to_hid_device(dev); \
	struct smff_status1_report status1; \
	u16 value; \
	if (!sm_read_status1(hid, &status1)) {\
		return sysfs_emit(buf, "-err-\n"); \
	} \
	value = le16_to_cpu((status1).struct_name); \
	return sysfs_emit(buf, "%u\n", value); \
} \
static DEVICE_ATTR_RO(name);

SM_SYSFS_STATUS1_ATTR_RO_U16(max_angle, max_angle);
SM_SYSFS_STATUS1_ATTR_RO_U8(ff_strength, ff_strength_percentage);
SM_SYSFS_STATUS1_ATTR_RO_U8(wheel_rotation_speed, wheel_rotation_speed);
SM_SYSFS_STATUS1_ATTR_RO_U8(mechanical_friction, mechanical_friction_percentage);
SM_SYSFS_STATUS1_ATTR_RO_U8(game_centering, game_centering);
SM_SYSFS_STATUS1_ATTR_RO_U8(game_inertia, game_inertia);
SM_SYSFS_STATUS1_ATTR_RO_U8(game_damper, game_damper);
SM_SYSFS_STATUS1_ATTR_RO_U8(game_friction, game_friction);
SM_SYSFS_STATUS1_ATTR_RO_U16(angle_lock, angle_lock);
SM_SYSFS_STATUS1_ATTR_RO_U8(feedback_detail, feedback_detail);
SM_SYSFS_STATUS1_ATTR_RO_U8(angle_lock_strength, angle_lock_strength);
SM_SYSFS_STATUS1_ATTR_RO_U8(filter_level, filter_level);
SM_SYSFS_STATUS1_ATTR_RO_U8(slew_rate_control, slew_rate_control);
SM_SYSFS_STATUS1_ATTR_RO_U8(wheel_channel, wheel_channel);

void simagic_ff_initsysfs(struct hid_device *hid) {
	struct smff_device *smff = get_smff_from_hid(hid);

	if (!smff)
		return;

	if (smff->sysfs_created)
		return;

	device_create_file(&hid->dev, &dev_attr_max_angle);
	device_create_file(&hid->dev, &dev_attr_ff_strength);
	device_create_file(&hid->dev, &dev_attr_wheel_rotation_speed);
	device_create_file(&hid->dev, &dev_attr_mechanical_friction);
	device_create_file(&hid->dev, &dev_attr_game_centering);
	device_create_file(&hid->dev, &dev_attr_game_inertia);
	device_create_file(&hid->dev, &dev_attr_game_damper);
	device_create_file(&hid->dev, &dev_attr_game_friction);
	device_create_file(&hid->dev, &dev_attr_angle_lock);
	device_create_file(&hid->dev, &dev_attr_feedback_detail);
	device_create_file(&hid->dev, &dev_attr_angle_lock_strength);
	device_create_file(&hid->dev, &dev_attr_filter_level);
	device_create_file(&hid->dev, &dev_attr_slew_rate_control);
	device_create_file(&hid->dev, &dev_attr_wheel_channel);
	smff->sysfs_created = true;
}

void simagic_ff_removesysfs(struct hid_device *hid) {
	struct smff_device *smff = get_smff_from_hid(hid);

	if (!smff)
		return;

	if (!smff->sysfs_created)
		return;

	device_remove_file(&hid->dev, &dev_attr_wheel_channel);
	device_remove_file(&hid->dev, &dev_attr_slew_rate_control);
	device_remove_file(&hid->dev, &dev_attr_filter_level);
	device_remove_file(&hid->dev, &dev_attr_angle_lock_strength);
	device_remove_file(&hid->dev, &dev_attr_feedback_detail);
	device_remove_file(&hid->dev, &dev_attr_angle_lock);
	device_remove_file(&hid->dev, &dev_attr_game_friction);
	device_remove_file(&hid->dev, &dev_attr_game_damper);
	device_remove_file(&hid->dev, &dev_attr_game_inertia);
	device_remove_file(&hid->dev, &dev_attr_game_centering);
	device_remove_file(&hid->dev, &dev_attr_mechanical_friction);
	device_remove_file(&hid->dev, &dev_attr_wheel_rotation_speed);
	device_remove_file(&hid->dev, &dev_attr_ff_strength);
	device_remove_file(&hid->dev, &dev_attr_max_angle);
	smff->sysfs_created = false;
}