#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "controller.h"
#include "gel/timer/timecheck.h"
#include "utils/utils.h"
#include "peripherals/rs485.h"
#include "peripherals/digin.h"
#include "peripherals/heartbeat.h"
#include "peripherals/digout.h"
#include "model/model.h"
#include "esp32c3_commandline.h"
#include "config/app_config.h"
#include "minion.h"
#include "esp_console.h"
#include "configuration.h"
#include "esp_log.h"
#include "device_commands.h"
#include "safety.h"
#include "rele.h"


static void    console_task(void *args);
static void    delay_ms(unsigned long ms);
static uint8_t get_inputs(void *args);


static easyconnect_interface_t context = {
    .save_serial_number = configuration_save_serial_number,
    .save_class         = configuration_save_class,
    .save_address       = configuration_save_address,
    .get_address        = model_get_address,
    .get_class          = model_get_class,
    .get_serial_number  = model_get_serial_number,
    .get_inputs         = get_inputs,
    .delay_ms           = delay_ms,
    .write_response     = rs485_write,
};

static const char *TAG = "Controller";


void controller_init(model_t *pmodel) {
    context.arg = pmodel;

    configuration_init(pmodel);
    minion_init(&context);

    static uint8_t      stack_buffer[APP_CONFIG_BASE_TASK_STACK_SIZE * 6];
    static StaticTask_t task_buffer;
    xTaskCreateStatic(console_task, "Console", sizeof(stack_buffer), &context, 1, stack_buffer, &task_buffer);
}


void controller_manage(model_t *pmodel) {
    static unsigned long timestamp = 0;

    minion_manage();

    if (is_expired(timestamp, get_millis(), 100UL)) {
        uint16_t class = model_get_class(pmodel);
        switch (CLASS_GET_MODE(class)) {
            case DEVICE_MODE_LIGHT:
                if (safety_ok()) {
                    heartbeat_set_state(HEARTBEAT_STATE_KO);
                    digout_update(DIGOUT_LIGHT_SIGNAL, !rele_is_on());
                } else {
                    heartbeat_set_state(HEARTBEAT_STATE_OK);
                    digout_update(DIGOUT_LIGHT_SIGNAL, 1);
                }
                break;
        }

        ESP_LOGD(TAG, "0x%X %i %i", class, digin_get(DIGIN_SAFETY), digin_get(DIGIN_SIGNAL));
        timestamp = get_millis();
    }
}


static void console_task(void *args) {
    const char              *prompt    = "EC-peripheral> ";
    easyconnect_interface_t *interface = args;

    esp32c3_commandline_init(interface);
    device_commands_register(interface->arg);
    esp_console_register_help_command();

    for (;;) {
        esp32c3_edit_cycle(prompt);
    }

    vTaskDelete(NULL);
}


static void delay_ms(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}


static uint8_t get_inputs(void *args) {
    (void)args;
    return (uint8_t)digin_get_inputs();
}
