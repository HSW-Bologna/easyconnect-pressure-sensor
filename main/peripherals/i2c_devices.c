#include "i2c_devices.h"
#include "i2c_common/i2c_common.h"
#include "i2c_ports/esp-idf/esp_idf_i2c_port.h"
#include "i2c_devices/temperature/MS5837/ms5837.h"


static void delay_ms(unsigned long ms);


i2c_driver_t press_driver = {
    .device_address = MS5837_DEFAULT_ADDRESS,
    .delay_ms       = delay_ms,
    .i2c_transfer   = esp_idf_i2c_port_transfer,
};


static void delay_ms(unsigned long ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}