#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/*
 * Get a device structure from a devicetree node with compatible
 * "maxim,ds18b20". (If there are multiple, just pick one.)
 */
static const struct device *get_ds18b20_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(maxim_ds18b20);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

static int cmd_read_temp(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = get_ds18b20_device();
	int res;

	if (dev == NULL) {
		return 0;
	}

    struct sensor_value temp;

    res = sensor_sample_fetch(dev);
    if (res != 0) {
        shell_print(shell,"sample_fetch() failed: %d\n", res);
        return res;
    }

    res = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (res != 0) {
        shell_print(shell,"channel_get() failed: %d\n", res);
        return res;
    }

    shell_print(shell,"Temp: %d.%06d\n", temp.val1, temp.val2);

	

	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor,
        SHELL_CMD(read_temp,NULL,"read temperature from sensor",cmd_read_temp),
      
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sensor,&sub_sensor,"sensor controller",NULL);
