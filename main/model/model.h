#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "easyconnect_interface.h"


#define EASYCONNECT_DEFAULT_MINION_ADDRESS       1
#define EASYCONNECT_DEFAULT_MINION_SERIAL_NUMBER 3
#define EASYCONNECT_DEFAULT_DEVICE_CLASS         CLASS(DEVICE_MODE_PRESSURE, DEVICE_GROUP_1)

#define GETTER_UNSAFE(name, field)                                                                                     \
    static inline __attribute__((always_inline)) typeof(((model_t *)0)->field) model_get_##name(model_t *pmodel) {     \
        assert(pmodel != NULL);                                                                                        \
        typeof(((model_t *)0)->field) res = pmodel->field;                                                             \
        return res;                                                                                                    \
    }

#define SETTER_UNSAFE(name, field)                                                                                     \
    static inline                                                                                                      \
        __attribute__((always_inline)) void model_set_##name(model_t *pmodel, typeof(((model_t *)0)->field) value) {   \
        assert(pmodel != NULL);                                                                                        \
        pmodel->field = value;                                                                                         \
    }

#define GETTER(type, name, field)                                                                                      \
    static inline __attribute__((always_inline)) typeof(((model_t *)0)->field) model_get_##name(type *arg) {           \
        model_t *pmodel = arg;                                                                                         \
        assert(pmodel != NULL);                                                                                        \
        xSemaphoreTake(pmodel->sem, portMAX_DELAY);                                                                    \
        typeof(((model_t *)0)->field) res = pmodel->field;                                                             \
        xSemaphoreGive(pmodel->sem);                                                                                   \
        return res;                                                                                                    \
    }


#define SETTER(type, name, field)                                                                                      \
    static inline                                                                                                      \
        __attribute__((always_inline)) void model_set_##name(type *arg, typeof(((model_t *)0)->field) value) {         \
        model_t *pmodel = arg;                                                                                         \
        assert(pmodel != NULL);                                                                                        \
        xSemaphoreTake(pmodel->sem, portMAX_DELAY);                                                                    \
        pmodel->field = value;                                                                                         \
        xSemaphoreGive(pmodel->sem);                                                                                   \
    }

#define GETTER_GENERIC(name, field) GETTER(void, name, field)
#define SETTER_GENERIC(name, field) SETTER(void, name, field)

#define GETTER_MODEL(name, field) GETTER(model_t, name, field)
#define SETTER_MODEL(name, field) SETTER(model_t, name, field)

#define GETTERNSETTER_GENERIC(name, field)                                                                             \
    GETTER_GENERIC(name, field)                                                                                        \
    SETTER_GENERIC(name, field)

#define GETTERNSETTER_UNSAFE(name, field)                                                                              \
    GETTER_UNSAFE(name, field)                                                                                         \
    SETTER_UNSAFE(name, field)

#define GETTERNSETTER(name, field)                                                                                     \
    GETTER_MODEL(name, field)                                                                                          \
    SETTER_MODEL(name, field)


typedef struct {
    StaticSemaphore_t semaphore_buffer;
    SemaphoreHandle_t sem;

    uint8_t missing_heartbeat;

    uint16_t address;
    uint16_t class;
    uint32_t serial_number;

    uint16_t minimum_pressure;
    uint16_t maximum_pressure;

    char minimum_pressure_message[EASYCONNECT_MESSAGE_SIZE + 1];
    char maximum_pressure_message[EASYCONNECT_MESSAGE_SIZE + 1];

    int16_t temperature;
    int16_t pressure;     // Pressure value in pascal
    int16_t humidity;
} model_t;


void     model_init(model_t *model);
uint16_t model_get_class(void *arg);
int      model_set_class(void *arg, uint16_t class, uint16_t *out_class);
uint8_t  model_is_pressure_ok(model_t *pmodel);
int      model_set_minimum_pressure(model_t *pmodel, uint16_t pressure);
int      model_set_maximum_pressure(model_t *pmodel, uint16_t pressure);
void     model_get_minimum_pressure_message(void *args, char *string);
void     model_set_minimum_pressure_message(model_t *pmodel, const char *string);
void     model_get_maximum_pressure_message(void *args, char *string);
void     model_set_maximum_pressure_message(model_t *pmodel, const char *string);


GETTERNSETTER_GENERIC(address, address);
GETTERNSETTER_GENERIC(serial_number, serial_number);
GETTERNSETTER_GENERIC(pressure, pressure);
GETTERNSETTER_GENERIC(temperature, temperature);
GETTERNSETTER_GENERIC(humidity, humidity);
GETTERNSETTER_GENERIC(missing_heartbeat, missing_heartbeat);
GETTER(model_t, minimum_pressure, minimum_pressure);
GETTER(model_t, maximum_pressure, maximum_pressure);

#endif