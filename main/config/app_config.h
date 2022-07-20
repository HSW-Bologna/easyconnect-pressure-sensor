#ifndef APP_CONFIG_H_INCLUDED
#define APP_CONFIG_H_INCLUDED

/*
 *  Macro riguardanti la configurazione dell'applicazione
 */

#define APP_CONFIG_FIRMWARE_VERSION_MAJOR 0
#define APP_CONFIG_FIRMWARE_VERSION_MINOR 1
#define APP_CONFIG_FIRMWARE_VERSION_PATCH 0

#define APP_CONFIG_BASE_TASK_STACK_SIZE 512

#define APP_CONFIG_HARDWARE_MODEL EASYCONNECT_DEVICE_SENSOR_PERIPHERAL

#define APP_CONFIG_MINIMUM_PRESSURE_THRESHOLD         300
#define APP_CONFIG_DEFAULT_MINIMUM_PRESSURE_THRESHOLD 400
#define APP_CONFIG_DEFAULT_MAXIMUM_PRESSURE_THRESHOLD 950
#define APP_CONFIG_MAXIMUM_PRESSURE_THRESHOLD         1200

#endif