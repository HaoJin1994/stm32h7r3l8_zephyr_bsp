
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


LOG_MODULE_REGISTER(led_shell,LOG_LEVEL_INF);

static int cmd_led_on(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if(!gpio_is_ready_dt(&led)){
        shell_print(sh,"gpio is not ready!");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&led,GPIO_OUTPUT_ACTIVE);
    if(ret < 0){
        shell_print(sh,"Failed to configure LED");
        return ret;
    }
    ret = gpio_pin_set_dt(&led, 1);
    if(ret < 0){
        shell_print(sh,"Failed to set gpio!");
    }


    shell_print(sh,"Turn led on!");
    return 0;
}

static int cmd_led_off(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if(!gpio_is_ready_dt(&led)){
        shell_print(sh,"gpio is not ready!");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&led,GPIO_OUTPUT_ACTIVE);
    if(ret < 0){
        shell_print(sh,"Failed to configure LED");
        return ret;
    }
    ret = gpio_pin_set_dt(&led, 0);
    if(ret < 0){
        shell_print(sh,"Failed to set gpio!");
    }

    shell_print(sh,"Turn led off!");

    return 0;
}

static int cmd_led_set(const struct shell *sh, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_error(sh, "Usage: led set <0/1>");
        return -EINVAL;
    }
    if(!gpio_is_ready_dt(&led)){
        shell_print(sh,"gpio is not ready!");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&led,GPIO_OUTPUT_ACTIVE);
    if(ret < 0){
        shell_print(sh,"Failed to configure LED");
        return ret;
    }
    int level = atoi(argv[1]);
    ret = gpio_pin_set_dt(&led, level);
    if(ret < 0){
        shell_print(sh,"Failed to set gpio!");
    }
    shell_print(sh,"Set led!");

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
        SHELL_CMD(on,NULL,"turn led on",cmd_led_on),
        SHELL_CMD(off,NULL,"turn led off",cmd_led_off),
        SHELL_CMD(set,NULL,"turn led on/off",cmd_led_set),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(led,&sub_led,"led controller",NULL);
