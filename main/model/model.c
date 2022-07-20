#include "model.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "easyconnect.h"
#include "easyconnect_interface.h"
#include "app_config.h"


static uint8_t valid_mode(uint16_t mode);


void model_init(model_t *pmodel) {
    pmodel->sem = xSemaphoreCreateMutexStatic(&pmodel->semaphore_buffer);

    pmodel->address       = EASYCONNECT_DEFAULT_MINION_ADDRESS;
    pmodel->serial_number = EASYCONNECT_DEFAULT_MINION_SERIAL_NUMBER;
    pmodel->class         = EASYCONNECT_DEFAULT_DEVICE_CLASS;

    pmodel->minimum_pressure = APP_CONFIG_DEFAULT_MINIMUM_PRESSURE_THRESHOLD;
    pmodel->maximum_pressure = APP_CONFIG_DEFAULT_MAXIMUM_PRESSURE_THRESHOLD;
}


uint16_t model_get_class(void *arg) {
    assert(arg != NULL);
    model_t *pmodel = arg;

    xSemaphoreTake(pmodel->sem, portMAX_DELAY);
    uint16_t result = (pmodel->class & CLASS_CONFIGURABLE_MASK) | (APP_CONFIG_HARDWARE_MODEL << 12);
    xSemaphoreGive(pmodel->sem);

    return result;
}


int model_set_class(void *arg, uint16_t class, uint16_t *out_class) {
    assert(arg != NULL);
    model_t *pmodel = arg;

    uint16_t corrected = class & CLASS_CONFIGURABLE_MASK;
    uint16_t mode      = CLASS_GET_MODE(corrected | (APP_CONFIG_HARDWARE_MODEL << 12));

    if (valid_mode(mode)) {
        if (out_class != NULL) {
            *out_class = corrected;
        }
        xSemaphoreTake(pmodel->sem, portMAX_DELAY);
        pmodel->class = corrected;
        xSemaphoreGive(pmodel->sem);
        return 0;
    } else {
        return -1;
    }
}


int model_set_minimum_pressure(model_t *pmodel, uint16_t pressure) {
    assert(pmodel != NULL);
    int res = 0;

    xSemaphoreTake(pmodel->sem, portMAX_DELAY);
    if (pressure > APP_CONFIG_DEFAULT_MINIMUM_PRESSURE_THRESHOLD &&
        pressure < APP_CONFIG_DEFAULT_MAXIMUM_PRESSURE_THRESHOLD) {
        pmodel->minimum_pressure = pressure;
    } else {
        res = -1;
    }
    xSemaphoreGive(pmodel->sem);

    return res;
}


int model_set_maximum_pressure(model_t *pmodel, uint16_t pressure) {
    assert(pmodel != NULL);
    int res = 0;

    xSemaphoreTake(pmodel->sem, portMAX_DELAY);
    if (pressure > APP_CONFIG_DEFAULT_MINIMUM_PRESSURE_THRESHOLD &&
        pressure < APP_CONFIG_DEFAULT_MAXIMUM_PRESSURE_THRESHOLD) {
        pmodel->maximum_pressure = pressure;
    } else {
        res = -1;
    }
    xSemaphoreGive(pmodel->sem);

    return res;
}


uint8_t model_is_pressure_ok(model_t *pmodel, uint16_t pressure) {
    assert(pmodel != NULL);
    uint8_t res = 0;

    xSemaphoreTake(pmodel->sem, portMAX_DELAY);
    res = pmodel->minimum_pressure < pressure && pressure < pmodel->minimum_pressure;
    xSemaphoreGive(pmodel->sem);

    return res;
}


static uint8_t valid_mode(uint16_t mode) {
    switch (mode) {
        case DEVICE_MODE_PRESSURE_SAFETY:
        case DEVICE_MODE_PRESSURE_TEMPERATURE_HUMIDITY_SAFETY:
        case DEVICE_MODE_TEMPERATURE_HUMIDITY_SAFETY:
            return 1;
        default:
            return 0;
    }
}