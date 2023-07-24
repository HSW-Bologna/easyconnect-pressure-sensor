#ifndef SENSORS_H_INCLUDED
#define SENSORS_H_INCLUDED


#include <stdint.h>


void    sensors_init(uint8_t pressure, uint8_t temperature_humidity);
void    sensors_read(double *temperature, double *pressure, double *humidity);
uint8_t sensors_get_errors(void);


#endif
