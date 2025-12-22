#include <linux/device.h>
#include <linux/hid.h>
#include "hid-simagic-sysfs.h"
#include "hid-simagic.h"

#define SIMAGIC_SYSFS_PERMISSION_RO (S_IRUSR | S_IRGRP | S_IROTH)
#define SIMAGIC_SYSFS_PERMISSION_RW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

static bool simagic_sysf_strtoint(int *out, const char* buf, size_t count)
{
	char tmp[32];

	if (count >= sizeof(tmp) || !out || !buf) {
		return false;
	}
	
	memcpy(tmp, buf, count);
	tmp[count] = '\0';

	return kstrtoint(tmp, 10, out) == 0;
}

static ssize_t simagic_attribute_status1_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf);

static ssize_t simagic_attribute_settings1_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count);

#define SM_SYSFS_ATTR_RO(name, show) \
	static DEVICE_ATTR(name, SIMAGIC_SYSFS_PERMISSION_RO, show, NULL);

#define SM_SYSFS_ATTR_RW(name, show, store) \
	static DEVICE_ATTR(name, SIMAGIC_SYSFS_PERMISSION_RW, show, store);

SM_SYSFS_ATTR_RW(max_angle, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(ff_strength, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(wheel_rotation_speed, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(mechanical_centering, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(mechanical_damper, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(center_damper, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(mechanical_friction, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(game_centering, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(game_inertia, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(game_damper, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RW(game_friction, simagic_attribute_status1_show, simagic_attribute_settings1_store);
SM_SYSFS_ATTR_RO(angle_lock, simagic_attribute_status1_show);
SM_SYSFS_ATTR_RO(feedback_detail, simagic_attribute_status1_show);
SM_SYSFS_ATTR_RO(angle_lock_strength, simagic_attribute_status1_show);
SM_SYSFS_ATTR_RO(filter_level, simagic_attribute_status1_show);
SM_SYSFS_ATTR_RO(slew_rate_control, simagic_attribute_status1_show);
SM_SYSFS_ATTR_RO(wheel_channel, simagic_attribute_status1_show);

static ssize_t simagic_attribute_status1_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct hid_device *hid = to_hid_device(dev);
	struct smff_status1_report status1;
	int value;

	if (!sm_read_status1(hid, &status1)) {
		return sysfs_emit(buf, "Error reading status\n");
	}

	if (attr == &dev_attr_max_angle)
		value = status1.max_angle;
	else if (attr == &dev_attr_ff_strength)
		value = status1.ff_strength;
	else if (attr == &dev_attr_wheel_rotation_speed)
		value = status1.wheel_rotation_speed;
	else if (attr == &dev_attr_mechanical_centering)
		value = status1.mechanical_centering;
	else if (attr == &dev_attr_mechanical_damper)
		value = status1.mechanical_damper;
	else if (attr == &dev_attr_center_damper)
		value = status1.center_damper;
	else if (attr == &dev_attr_mechanical_friction)
		value = status1.mechanical_friction;
	else if (attr == &dev_attr_game_centering)
		value = status1.game_centering;
	else if (attr == &dev_attr_game_inertia)
		value = status1.game_inertia;
	else if (attr == &dev_attr_game_damper)
		value = status1.game_damper;
	else if (attr == &dev_attr_game_friction)
		value = status1.game_friction;
	else if (attr == &dev_attr_angle_lock)
		value = status1.angle_lock;
	else if (attr == &dev_attr_feedback_detail)
		value = status1.feedback_detail;
	else if (attr == &dev_attr_angle_lock_strength)
		value = status1.angle_lock_strength;
	else if (attr == &dev_attr_filter_level)
		value = status1.filter_level;
	else if (attr == &dev_attr_slew_rate_control)
		value = status1.slew_rate_control;
	else if (attr == &dev_attr_wheel_channel)
		value = status1.wheel_channel;
	else
		return sysfs_emit(buf, "Unknown attribute\n");

	return sysfs_emit(buf, "%d\n", value);
}

static ssize_t simagic_attribute_settings1_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct hid_device *hid = to_hid_device(dev);
	struct smff_settings1_report settings1;
	int value;

	if (!simagic_sysf_strtoint(&value, buf, count))
		return count;
	
	if (!sm_read_settings1(hid, &settings1))
		return count;
	
	if (attr == &dev_attr_max_angle)
		settings1.max_angle = cpu_to_le16((u16)value);
	else if (attr == &dev_attr_ff_strength)
		settings1.ff_strength = cpu_to_le16((u16)value);
	else if (attr == &dev_attr_wheel_rotation_speed)
		settings1.wheel_rotation_speed = value;
	else if (attr == &dev_attr_mechanical_centering)
		settings1.mechanical_centering = value;
	else if (attr == &dev_attr_mechanical_damper)
		settings1.mechanical_damper = value;
	else if (attr == &dev_attr_center_damper)
		settings1.center_damper = value;
	else if (attr == &dev_attr_mechanical_friction)
		settings1.mechanical_friction = value;
	else if (attr == &dev_attr_game_centering)
		settings1.game_centering = value;
	else if (attr == &dev_attr_game_inertia)
		settings1.game_inertia = value;
	else if (attr == &dev_attr_game_damper)
		settings1.game_damper = value;
	else if (attr == &dev_attr_game_friction)
		settings1.game_friction = value;
	else
		return count;
	
	sm_write_settings1(hid, &settings1);
	
	return count;
}

void simagic_ff_initsysfs(struct hid_device *hid) {
	struct smff_device *smff = get_smff_from_hid(hid);

	if (!smff)
		return;

	if (smff->sysfs_created)
		return;

	device_create_file(&hid->dev, &dev_attr_max_angle);
	device_create_file(&hid->dev, &dev_attr_ff_strength);
	device_create_file(&hid->dev, &dev_attr_wheel_rotation_speed);
	device_create_file(&hid->dev, &dev_attr_mechanical_centering);
	device_create_file(&hid->dev, &dev_attr_mechanical_damper);
	device_create_file(&hid->dev, &dev_attr_center_damper);
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
	device_remove_file(&hid->dev, &dev_attr_center_damper);
	device_remove_file(&hid->dev, &dev_attr_mechanical_damper);
	device_remove_file(&hid->dev, &dev_attr_mechanical_centering);
	device_remove_file(&hid->dev, &dev_attr_wheel_rotation_speed);
	device_remove_file(&hid->dev, &dev_attr_ff_strength);
	device_remove_file(&hid->dev, &dev_attr_max_angle);
	smff->sysfs_created = false;
}