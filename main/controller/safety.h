#ifndef SAFETY_H_INCLUDED
#define SAFETY_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


uint8_t safety_signal_ok(model_t *pmodel);
uint8_t safety_pressure_ok(model_t *pmodel);


#endif