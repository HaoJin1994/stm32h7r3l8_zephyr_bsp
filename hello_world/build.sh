#!/bin/bash

# 读取模块配置

OVERLAYS="app.overlay"
CONFIGS="prj.conf"

# mcuboot 模块
#CONFIGS="${CONFIGS};modules/mcuboot/mcuboot.conf"

# led  模块
#OVERLAYS="${OVERLAYS};modules/led/led.overlay"
#CONFIGS="${CONFIGS};modules/led/led.conf"

# cdc_acm 模块
#OVERLAYS="${OVERLAYS};modules/cdc_acm/cdc_acm.overlay"
#CONFIGS="${CONFIGS};modules/cdc_acm/cdc_acm.conf"

# pwm led模块
#OVERLAYS="${OVERLAYS};modules/pwm/pwm.overlay"
#CONFIGS="${CONFIGS};modules/pwm/pwm.conf"

#spi_flash 模块
#OVERLAYS="${OVERLAYS};modules/spi_flash/spi_flash.overlay"
#CONFIGS="${CONFIGS};modules/spi_flash/spi_flash.conf" 

# i2c_eeprom 模块
#OVERLAYS="${OVERLAYS};modules/eeprom/eeprom.overlay"
#CONFIGS="${CONFIGS};modules/eeprom/eeprom.conf"

# adc 模块
#OVERLAYS="${OVERLAYS};modules/adc/adc.overlay"
#CONFIGS="${CONFIGS};modules/adc/adc.conf"

#dac 模块
#OVERLAYS="${OVERLAYS};modules/dac/dac.overlay"
#CONFIGS="${CONFIGS};modules/dac/dac.conf"

#net 模块
OVERLAYS="${OVERLAYS};modules/net/net.overlay"
CONFIGS="${CONFIGS};modules/net/net.conf"

#OVERLAYS=${OVERLAYS#;}

echo "=== Build Configuration ==="
echo "Overlays: ${OVERLAYS}"
echo "Configs: ${CONFIGS}"
echo "=========================="

west build -b aknw_stm32h7r3/stm32h7r3xx/ext_flash_app --sysbuild -p always -- \
  -DBOARD_ROOT=/home/ubuntu/work/aknw_h7rsx/ \
  -DDTC_OVERLAY_FILE="${OVERLAYS}" \
  -DCONF_FILE="${CONFIGS}"
#stm32h7r3xx/ext_flash_app