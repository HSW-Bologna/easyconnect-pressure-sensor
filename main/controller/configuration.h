#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


void configuration_init(model_t *pmodel);
void configuration_save_serial_number(void *args, uint32_t value);
int  configuration_save_class(void *args, uint16_t value);
void configuration_save_address(void *args, uint16_t value);
int  configuration_save_minimum_pressure(void *args, uint16_t value);
int  configuration_save_maximum_pressure(void *args, uint16_t value);
void configuration_save_minimum_pressure_message(void *args, const char *string);
void configuration_save_maximum_pressure_message(void *args, const char *string);


#endif