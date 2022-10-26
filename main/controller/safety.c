#include "peripherals/digin.h"
#include "sensors.h"
#include "model/model.h"


uint8_t safety_signal_ok(model_t *pmodel) {
    return digin_get(DIGIN_SAFETY) == 0;
}


uint8_t safety_pressure_ok(model_t *pmodel) {
    return model_is_pressure_ok(pmodel);
}