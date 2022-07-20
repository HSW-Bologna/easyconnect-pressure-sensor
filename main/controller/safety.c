#include "peripherals/digin.h"
#include "sensors.h"
#include "model/model.h"


uint8_t safety_ok(model_t *pmodel) {
    double temperature = 0;
    double pressure    = 0;

    sensors_read(&temperature, &pressure);

    uint16_t mbar_pressure = (uint16_t)pressure * 1000;

    return digin_get(DIGIN_SAFETY) != 0 && model_is_pressure_ok(pmodel, mbar_pressure);
}