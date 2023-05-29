#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "esp_log.h"
#include "model/model.h"
#include "peripherals/storage.h"
#include "easyconnect_interface.h"
#include "configuration.h"


#define ADDRESS_KEY                  "indirizzo"
#define SERIAL_NUM_KEY               "numeroseriale"
#define MODEL_KEY                    "CLASS"
#define MINIMUM_PRESSURE_KEY         "MINPRESS"
#define MAXIMUM_PRESSURE_KEY         "MAXPRESS"
#define MINIMUM_PRESSURE_MESSAGE_KEY "MINPRESSMSG"
#define MAXIMUM_PRESSURE_MESSAGE_KEY "MAXPRESSMSG"


void configuration_init(model_t *pmodel) {
    uint16_t value = 0;
    uint32_t value_32bit = 0;

    if (load_uint16_option(&value, ADDRESS_KEY) == 0) {
        model_set_address(pmodel, value);
    }
    if (load_uint32_option(&value_32bit, SERIAL_NUM_KEY) == 0) {
        model_set_serial_number(pmodel, value_32bit);
    }
    if (load_uint16_option(&value, MODEL_KEY) == 0) {
        model_set_class(pmodel, value, NULL);
    }
    if (load_uint16_option(&value, MINIMUM_PRESSURE_KEY) == 0) {
        model_set_minimum_pressure(pmodel, value);
    }
    if (load_uint16_option(&value, MAXIMUM_PRESSURE_KEY) == 0) {
        model_set_maximum_pressure(pmodel, value);
    }

    load_blob_option(pmodel->minimum_pressure_message, sizeof(pmodel->minimum_pressure_message),
                     MINIMUM_PRESSURE_MESSAGE_KEY);
    load_blob_option(pmodel->maximum_pressure_message, sizeof(pmodel->maximum_pressure_message),
                     MAXIMUM_PRESSURE_MESSAGE_KEY);
}


void configuration_save_serial_number(void *args, uint32_t value) {
    save_uint32_option(&value, SERIAL_NUM_KEY);
    model_set_serial_number(args, value);
}


int configuration_save_class(void *args, uint16_t value) {
    uint16_t corrected;
    if (model_set_class(args, value, &corrected) == 0) {
        save_uint16_option(&corrected, MODEL_KEY);
        return 0;
    } else {
        return -1;
    }
}


void configuration_save_address(void *args, uint16_t value) {
    save_uint16_option(&value, ADDRESS_KEY);
    model_set_address(args, value);
}


int configuration_save_minimum_pressure(void *args, uint16_t value) {
    if (model_set_minimum_pressure(args, value) == 0) {
        save_uint16_option(&value, MINIMUM_PRESSURE_KEY);
        return 0;
    } else {
        return -1;
    }
}


int configuration_save_maximum_pressure(void *args, uint16_t value) {
    if (model_set_maximum_pressure(args, value) == 0) {
        save_uint16_option(&value, MAXIMUM_PRESSURE_KEY);
        return 0;
    } else {
        return -1;
    }
}


void configuration_save_minimum_pressure_message(void *args, const char *string) {
    save_blob_option((char *)string, strlen(string), MINIMUM_PRESSURE_MESSAGE_KEY);
    model_set_minimum_pressure_message(args, string);
}


void configuration_save_maximum_pressure_message(void *args, const char *string) {
    save_blob_option((char *)string, strlen(string), MAXIMUM_PRESSURE_MESSAGE_KEY);
    model_set_maximum_pressure_message(args, string);
}