#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include "main.h"

LOG_MODULE_REGISTER(main,LOG_LEVEL_DBG);

#define SLEEP_TIME_MS   2000

#ifdef CONFIG_MODULE_LED
#define LED0_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

#define SPI_FLASH_COMPAT jedec_spi_nor



int main(void)
{

    k_msleep(SLEEP_TIME_MS);
    printk("Application started\n");
  
    printk("UART interrupt mode enabled\n");


    //littlefs_example();
    //flash_test();
    //k_msleep(SLEEP_TIME_MS);
    //print_thread_stats();
#ifdef CONFIG_MODULE_LED
    if(!gpio_is_ready_dt(&led)){
        printk("gpio is not ready\n");
        return -1;
    }
    int ret = gpio_pin_configure_dt(&led,GPIO_OUTPUT_ACTIVE);
    if(ret < 0){
        printk("Failed to configure LED\n");
        return ret;
    }
#endif


#if 0
 
    char data[32] = {0};

    const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_FLASH_COMPAT);
    if(!device_is_ready(flash_dev)){
        printk("SPI flash device is not ready\n");
        return -1;
    }
    printk("SPI flash device is ready\n");
    int rc = flash_erase(flash_dev,0x0,0x1000);
    if(rc < 0){
        printk("Failed to erase flash\n");
        return rc;
    }
    rc = flash_write(flash_dev,0x0,&"hello world",12);
    if(rc < 0){
        printk("Failed to write flash\n");
        return rc;
    }
    printk("Flash write success\n");
    rc = flash_read(flash_dev,0x0,data,12);
    if(rc < 0){
        printk("Failed to read flash\n");
        return rc;
    }
    printk("Read data: %s\n",data);

#endif

    while(1){
        //闪烁LED
#ifdef CONFIG_MODULE_LED
        gpio_pin_toggle_dt(&led);
#endif
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;

}