#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "hardwareprofile.h"
#include "heartbeat.h"


static const char   *TAG   = "Heartbeat";
static TimerHandle_t timer = NULL;


static void heartbeat_timer(TimerHandle_t timer) {
    heartbeat_state_t state = (heartbeat_state_t)(uintptr_t)pvTimerGetTimerID(timer);

    switch (state) {
        case HEARTBEAT_STATE_OK:
            gpio_set_level(HAP_LED_RUN, 0);
            break;

        case HEARTBEAT_STATE_KO:
            gpio_set_level(HAP_LED_RUN, 1);
            break;
    }
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
    gpio_set_level(HAP_LED_RUN, 1);

    timer = xTimerCreate("idle", pdMS_TO_TICKS(100), pdTRUE, (void *)(uintptr_t)HEARTBEAT_STATE_OK, heartbeat_timer);
    xTimerStart(timer, portMAX_DELAY);
}


void heartbeat_set_state(heartbeat_state_t state) {
    vTimerSetTimerID(timer, (void *)(uintptr_t)state);
}