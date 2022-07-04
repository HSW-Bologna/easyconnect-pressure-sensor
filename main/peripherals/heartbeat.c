#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "hardwareprofile.h"
#include "heartbeat.h"


static const char       *TAG    = "Heartbeat";
static TimerHandle_t     timer  = NULL;
static SemaphoreHandle_t sem    = NULL;
static heartbeat_state_t state  = HEARTBEAT_STATE_OK;
static uint32_t          blinks = 0;


static void heartbeat_timer(TimerHandle_t timer) {
    xSemaphoreTake(sem, portMAX_DELAY);
    if (blinks > 0) {
        gpio_set_level(HAP_LED_RUN, (blinks % 2) != 0);
        blinks--;
    } else {
        switch (state) {
            case HEARTBEAT_STATE_OK:
                gpio_set_level(HAP_LED_RUN, 1);
                break;

            case HEARTBEAT_STATE_KO:
                gpio_set_level(HAP_LED_RUN, 0);
                break;
        }
    }
    xSemaphoreGive(sem);
}


void heartbeat_init(void) {
    (void)TAG;

    gpio_config_t config = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(HAP_LED_RUN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    gpio_set_level(HAP_LED_RUN, 0);

    static StaticSemaphore_t semaphore_buffer;
    sem = xSemaphoreCreateMutexStatic(&semaphore_buffer);

    timer = xTimerCreate("idle", pdMS_TO_TICKS(100), pdTRUE, NULL, heartbeat_timer);
    xTimerStart(timer, portMAX_DELAY);
}


void heartbeat_set_state(heartbeat_state_t new_state) {
    xSemaphoreTake(sem, portMAX_DELAY);
    state = new_state;
    xSemaphoreGive(sem);
}


void heartbeat_signal(uint32_t new_blinks) {
    xSemaphoreTake(sem, portMAX_DELAY);
    blinks = new_blinks * 2;
    xSemaphoreGive(sem);
}