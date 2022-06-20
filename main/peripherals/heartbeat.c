#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "hardwareprofile.h"
#include "heartbeat.h"


static void heartbeat_print_heap_status(void);


static const char   *TAG       = "Heartbeat";
static TimerHandle_t timer     = NULL;
static unsigned long hb_period = 1000UL;

/**
 * Timer di attivita'. Accende e spegne il led di attivita'
 */
static void heartbeat_timer(void *arg) {
    (void)arg;
    static int    blink   = 0;
    static size_t counter = 0;

    gpio_set_level(HAP_LED_RUN, !blink);
    blink = !blink;

    if (counter++ >= 10) {
        heartbeat_print_heap_status();
        counter = 0;
    }

    xTimerChangePeriod(timer, pdMS_TO_TICKS(blink ? hb_period : 50UL), portMAX_DELAY);
}


void heartbeat_init(size_t period_ms) {
    gpio_set_direction(HAP_LED_RUN, GPIO_MODE_OUTPUT);
    hb_period = period_ms;
    timer     = xTimerCreate("idle", pdMS_TO_TICKS(hb_period), pdTRUE, NULL, heartbeat_timer);
    xTimerStart(timer, portMAX_DELAY);
    heartbeat_print_heap_status();
}


void heartbeat_stop(void) {
    xTimerStop(timer, portMAX_DELAY);
}


void heartbeat_resume(void) {
    xTimerStart(timer, portMAX_DELAY);
}


static void heartbeat_print_heap_status(void) {
    ESP_LOGI(TAG, "Low water mark: %i, free memory %i", xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());
}