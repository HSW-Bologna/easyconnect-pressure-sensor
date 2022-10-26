#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "controller.h"
#include "gel/timer/timecheck.h"
#include "utils/utils.h"
#include "peripherals/rs485.h"
#include "peripherals/digin.h"
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
#include "approval.h"
#include "sensors.h"
#include "leds_communication.h"
#include "leds_activity.h"


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
    (void)TAG;
    context.arg = pmodel;

    sensors_init();

    configuration_init(pmodel);
    minion_init(&context);

    static uint8_t      stack_buffer[APP_CONFIG_BASE_TASK_STACK_SIZE * 6];
    static StaticTask_t task_buffer;
    xTaskCreateStatic(console_task, "Console", sizeof(stack_buffer), &context, 1, stack_buffer, &task_buffer);
}


void controller_manage(model_t *pmodel) {
    static unsigned long timestamp = 0;

    minion_manage();

    if (is_expired(timestamp, get_millis(), 500UL) || digin_is_value_ready()) {
        double temperature = 0;
        double pressure    = 0;

        sensors_read(&temperature, &pressure);
        int16_t mbar_pressure = (int16_t)((pressure - 1000.) * 10);
        (void)temperature;
        model_set_pressure(pmodel, mbar_pressure);

        uint8_t safety_signal   = safety_signal_ok(pmodel);
        uint8_t safety_pressure = safety_pressure_ok(pmodel);

        if (safety_signal && safety_pressure && !model_get_missing_heartbeat(pmodel)) {
            approval_on();
        } else {
            approval_off();
        }

        timestamp = get_millis();
    }

    digout_update(DIGOUT_LED_APPROVAL, (leds_communication_manage(get_millis(), !model_get_missing_heartbeat(pmodel))));
    digout_update(DIGOUT_LED_SAFETY,
                  (leds_activity_manage(get_millis(), safety_pressure_ok(pmodel), safety_signal_ok(pmodel), 1)));
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
