#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "model/model.h"
#include "controller/controller.h"
#include "peripherals/digin.h"
#include "peripherals/digout.h"
#include "peripherals/minion.h"
#include "peripherals/storage.h"

static const char *TAG = "Main";

void app_main(void) {
    model_t model;
    storage_init();
    model_init(&model);
    // view_init(&model);
    controller_init(&model);
    digin_init();
    digout_init();
    minion_init();

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        minion_manage();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
