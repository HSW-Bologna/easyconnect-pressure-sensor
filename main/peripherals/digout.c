#include "peripherals/hardwareprofile.h"
#include "peripherals/digout.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"


static const char *TAG = "Digout";


void digout_init(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    io_conf.mode          = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask  = BIT64(HAP_APPROVAL) | BIT64(HAP_LED_SAFETY) | BIT64(HAP_LED_APPROVAL);
    io_conf.pull_down_en  = 0;
    io_conf.pull_up_en    = 0;
    gpio_config(&io_conf);

    gpio_set_level(HAP_APPROVAL, 0);
    gpio_set_level(HAP_LED_APPROVAL, 1);
    gpio_set_level(HAP_LED_SAFETY, 1);

    ESP_LOGI(TAG, "Digout initialized");
}


void digout_update(digout_t digout, uint8_t val) {
    val               = val > 0;
    gpio_num_t gpio[] = {HAP_APPROVAL, HAP_LED_SAFETY, HAP_LED_APPROVAL};
    gpio_set_level(gpio[digout], !val);
}


uint8_t digout_get(void) {
    return gpio_get_level(HAP_APPROVAL);
}
