#include <driver/i2c.h>
#include <esp_random.h>
#include <bootloader_random.h>
#include <esp_system.h>
#include "hardwareprofile.h"


void system_random_init(void) {
    /* Initialize RNG address */
    bootloader_random_enable();
    srand(esp_random());
    bootloader_random_disable();
}


void system_i2c_init(void) {
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_SDA,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_io_num       = I2C_SCL,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
        .clk_flags        = 0,
    };
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
}