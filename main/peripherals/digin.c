#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "peripherals/hardwareprofile.h"
#include "peripherals/digin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "gel/debounce/debounce.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "digin.h"

static debounce_filter_t filter;
static SemaphoreHandle_t sem;


static void periodic_read(TimerHandle_t timer);


void digin_init(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask  = BIT64(HAP_SAFETY) | BIT64(HAP_SIGNAL);
    io_conf.mode          = GPIO_MODE_INPUT;
    io_conf.pull_down_en  = 0;
    io_conf.pull_up_en    = 0;
    gpio_config(&io_conf);

    debounce_filter_init(&filter);
    sem = xSemaphoreCreateMutex();

    TimerHandle_t timer = xTimerCreate("timerInput", pdMS_TO_TICKS(5), pdTRUE, NULL, periodic_read);
    xTimerStart(timer, portMAX_DELAY);
}


int digin_get(digin_t digin) {
    int res = 0;
    xSemaphoreTake(sem, portMAX_DELAY);
    res = debounce_read(&filter, digin);
    xSemaphoreGive(sem);
    return res;
}


int digin_take_reading(void) {
    unsigned int input = 0;
    input |= !gpio_get_level(HAP_SAFETY);
    input |= (!gpio_get_level(HAP_SIGNAL)) << 1;
    return debounce_filter(&filter, input, 10);
}


unsigned int digin_get_inputs(void) {
    return debounce_value(&filter);
}


static void periodic_read(TimerHandle_t timer) {
    (void)timer;
    xSemaphoreTake(sem, portMAX_DELAY);
    digin_take_reading();
    xSemaphoreGive(sem);
}
