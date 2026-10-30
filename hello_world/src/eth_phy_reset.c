#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/mii.h>

#define PHY_RESET_NODE DT_NODELABEL(phy_reset_pin)
static const struct gpio_dt_spec phy_reset =
    GPIO_DT_SPEC_GET(PHY_RESET_NODE, gpios);

static int eth_phy_early_reset(void)
{
    int ret;

    const struct device *mdio_dev = DEVICE_DT_GET(DT_NODELABEL(mdio));
    const struct device *eth_dev = DEVICE_DT_GET(DT_NODELABEL(mac));
#if 0
    if (!gpio_is_ready_dt(&phy_reset)) {
        printk("[ETH_RESET] ERROR: PHY reset GPIO not ready\n");
        return -ENODEV;
    }
    k_msleep(2000);
    printk("[ETH_RESET] Configuring reset pin (PD3)...\n");
    /* GPIO_OUTPUT_INACTIVE： inactive=逻辑0 → 物理高，默认不复位 */
    ret = gpio_pin_configure_dt(&phy_reset, GPIO_OUTPUT_INACTIVE );
    if (ret < 0) {
        printk("[ETH_RESET] ERROR: Failed to configure GPIO: %d\n", ret);
        return ret;
    }

    k_msleep(20);
    int pin_val = gpio_pin_get_dt(&phy_reset);
    printk("[ETH_RESET] Get pin logic level: %d\n", pin_val);

    //配置为输入
    ret = gpio_pin_configure_dt(&phy_reset, GPIO_DISCONNECTED);
    if(ret < 0){
        printk("[ETH_RESET] ERROR: Failed to configure GPIO: %d\n", ret);
        return ret;
    }
    printk("[ETH_RESET] config to input...\n");
    pin_val = gpio_pin_get_dt(&phy_reset);
    printk("[ETH_RESET] Get pin logic level: %d\n", pin_val);
#endif
    printk("[ETH_RESET] Waiting for PHY PLL & 50MHz REF_CLK (2000ms)...\n");
    k_msleep(2000);
    printk("[ETH_RESET] YT8512C PHY reset completed\n");

    ret = device_init(mdio_dev);
    printk("[ETH_RESET] MDIO device_init ret: %d\n", ret);


    /* 手动初始化ETH MAC */
    ret = device_init(eth_dev);
    printk("[ETH_RESET] ETH MAC device_init ret: %d\n", ret);
    return 0;
}
/* 优先级 50 < CONFIG_ETH_INIT_PRIORITY=60 */
SYS_INIT(eth_phy_early_reset, POST_KERNEL, 50);
