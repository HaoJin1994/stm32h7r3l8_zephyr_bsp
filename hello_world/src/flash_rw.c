#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include "main.h"

LOG_MODULE_REGISTER(flash_test, LOG_LEVEL_INF);

void flash_test(void)
{
    const struct flash_area *fa;
    uint8_t write_buf[256];
    uint8_t read_buf[256];
    int rc;

    // 打开 Flash 分区
    rc = flash_area_open(FIXED_PARTITION_ID(slot1_partition), &fa);
    if (rc != 0) {
        LOG_ERR("Failed to open flash area: %d", rc);
        return;
    }

    // 擦除 Flash（必须先擦除才能写入）
    rc = flash_area_erase(fa, 0, fa->fa_size);
    if (rc != 0) {
        LOG_ERR("Failed to erase flash: %d", rc);
        flash_area_close(fa);
        return;
    }

    // 准备写入数据
    for (int i = 0; i < sizeof(write_buf); i++) {
        write_buf[i] = i;
    }

    // 写入数据
    rc = flash_area_write(fa, 0, write_buf, sizeof(write_buf));
    if (rc != 0) {
        LOG_ERR("Failed to write flash: %d", rc);
        flash_area_close(fa);
        return;
    }

    // 读取数据
    rc = flash_area_read(fa, 0, read_buf, sizeof(read_buf));
    if (rc != 0) {
        LOG_ERR("Failed to read flash: %d", rc);
        flash_area_close(fa);
        return;
    }

    // 验证数据
    if (memcmp(write_buf, read_buf, sizeof(write_buf)) == 0) {
        LOG_INF("Flash read/write successful");
    } else {
        LOG_ERR("Flash data mismatch");
    }

    // 关闭 Flash 分区
    flash_area_close(fa);
}