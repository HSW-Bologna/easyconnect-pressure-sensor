#ifndef SENSORS_H_INCLUDED
#define SENSORS_H_INCLUDED


void sensors_init(uint8_t pressure, uint8_t temperature_humidity);
void sensors_read(double *temperature, double *pressure, double *humidity);


#endif