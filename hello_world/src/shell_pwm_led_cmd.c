#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>

#define LED_PWM_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(pwm_leds)

static const struct device *led_pwm = DEVICE_DT_GET(LED_PWM_NODE_ID);

static int cmd_pwm_led_on(const struct shell *shell, size_t argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    if(!device_is_ready(led_pwm)) {
        shell_print(shell,"LED PWM device is not ready");
        return 0;
    } 

    int err = led_on(led_pwm,0);
    if(err) {
        shell_print(shell,"Error turning on LED PWM device");
        return 0;
    }
    shell_print(shell,"LED PWM device turned on");
}

static int cmd_pwm_led_set_brightness(const struct shell *shell, size_t argc, char **argv)
{
    UNUSED(argc);
    if(!device_is_ready(led_pwm)) {
        shell_print(shell,"LED PWM device is not ready");
        return 0;
    } 
    int err = led_set_brightness(led_pwm,0,atoi(argv[1]));
    if(err) {
        shell_print(shell,"Error setting brightness");
        return 0;
    }
    shell_print(shell,"Brightness set to %d",atoi(argv[1]));
}

static int cmd_pwm_led_off(const struct shell *shell, size_t argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    if(!device_is_ready(led_pwm)) {
        shell_print(shell,"LED PWM device is not ready");
        return 0;
    } 
    int err = led_off(led_pwm,0);
    if(err) {
        shell_print(shell,"Error turning off LED PWM device");
        return 0;
    }
    shell_print(shell,"LED PWM device turned off");
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_pwm_led,
        SHELL_CMD(on,NULL,"turn led on",cmd_pwm_led_on),
        SHELL_CMD(off,NULL,"turn led off",cmd_pwm_led_off),
        SHELL_CMD(set_brightness,NULL,"set led brightness",cmd_pwm_led_set_brightness),
        
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pwm_led,&sub_pwm_led,"pwm led controller",NULL);
