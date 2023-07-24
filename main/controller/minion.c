#include <sys/time.h>
#include <assert.h>
#include "minion.h"
#include "config/app_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/projdefs.h"
#include "peripherals/hardwareprofile.h"
#include "lightmodbus/base.h"
#include "lightmodbus/lightmodbus.h"
#include "lightmodbus/slave.h"
#include "lightmodbus/slave_func.h"
#include "gel/timer/timecheck.h"
#include "utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "configuration.h"
#include "peripherals/digout.h"
#include "peripherals/digin.h"
#include "peripherals/rs485.h"
#include "easyconnect.h"
#include "model/model.h"
#include "safety.h"
#include "config/app_config.h"
#include "gel/serializer/serializer.h"
#include "sensors.h"


#define HOLDING_REGISTER_MINIMUM_PRESSURE_MESSAGE EASYCONNECT_HOLDING_REGISTER_MESSAGE_1
#define HOLDING_REGISTER_MAXIMUM_PRESSURE_MESSAGE                                                                      \
    (HOLDING_REGISTER_MINIMUM_PRESSURE_MESSAGE + EASYCONNECT_MESSAGE_NUM_REGISTERS)
#define HOLDING_REGISTER_PRESSURE    EASYCONNECT_HOLDING_REGISTER_CUSTOM_START
#define HOLDING_REGISTER_TEMPERATURE EASYCONNECT_HOLDING_REGISTER_CUSTOM_START + 1
#define HOLDING_REGISTER_HUMIDITY    EASYCONNECT_HOLDING_REGISTER_CUSTOM_START + 2


static const char   *TAG = "Minion";
ModbusSlave          minion;
static unsigned long timestamp = 0;

static ModbusError           register_callback(const ModbusSlave *status, const ModbusRegisterCallbackArgs *args,
                                               ModbusRegisterCallbackResult *result);
static ModbusError           exception_callback(const ModbusSlave *minion, uint8_t function, ModbusExceptionCode code);
static LIGHTMODBUS_RET_ERROR initialization_function(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                                     uint8_t requestLength);
static LIGHTMODBUS_RET_ERROR set_datetime(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                          uint8_t requestLength);
static LIGHTMODBUS_RET_ERROR heartbeat_received(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                                uint8_t requestLength);


static const ModbusSlaveFunctionHandler custom_functions[] = {
#if defined(LIGHTMODBUS_F01S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {1, modbusParseRequest01020304},
#endif
#if defined(LIGHTMODBUS_F02S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {2, modbusParseRequest01020304},
#endif
#if defined(LIGHTMODBUS_F03S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {3, modbusParseRequest01020304},
#endif
#if defined(LIGHTMODBUS_F04S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {4, modbusParseRequest01020304},
#endif
#if defined(LIGHTMODBUS_F05S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {5, modbusParseRequest0506},
#endif
#if defined(LIGHTMODBUS_F06S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {6, modbusParseRequest0506},
#endif
#if defined(LIGHTMODBUS_F15S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {15, modbusParseRequest1516},
#endif
#if defined(LIGHTMODBUS_F16S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {16, modbusParseRequest1516},
#endif
#if defined(LIGHTMODBUS_F22S) || defined(LIGHTMODBUS_SLAVE_FULL)
    {22, modbusParseRequest22},
#endif

    {EASYCONNECT_FUNCTION_CODE_CONFIG_ADDRESS, easyconnect_set_address_function},
    {EASYCONNECT_FUNCTION_CODE_RANDOM_SERIAL_NUMBER, easyconnect_send_address_function},
    {EASYCONNECT_FUNCTION_CODE_SET_TIME, set_datetime},
    {EASYCONNECT_FUNCTION_CODE_HEARTBEAT, heartbeat_received},
    {EASYCONNECT_FUNCTION_CODE_NETWORK_INITIALIZATION, initialization_function},

    // Guard - prevents 0 array size
    {0, NULL},
};


void minion_init(easyconnect_interface_t *context) {
    ModbusErrorInfo err;
    err = modbusSlaveInit(&minion,
                          register_callback,          // Callback for register operations
                          exception_callback,         // Callback for handling minion exceptions (optional)
                          modbusDefaultAllocator,     // Memory allocator for allocating responses
                          custom_functions,           // Set of supported functions
                          14                          // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusSlaveInit() failed");

    modbusSlaveSetUserPointer(&minion, context);

    timestamp = get_millis();
}


void minion_manage(void) {
    uint8_t buffer[256] = {0};
    int     len         = rs485_read(buffer, 256);

    easyconnect_interface_t *context = modbusSlaveGetUserPointer(&minion);

    if (len > 0) {
        // ESP_LOG_BUFFER_HEX(TAG, buffer, len);

        ModbusErrorInfo err;
        err = modbusParseRequestRTU(&minion, context->get_address(context->arg), buffer, len);

        if (modbusIsOk(err)) {
            size_t rlen = modbusSlaveGetResponseLength(&minion);
            if (rlen > 0) {
                rs485_write((uint8_t *)modbusSlaveGetResponse(&minion), rlen);
            } else {
                ESP_LOGD(TAG, "Empty response");
            }
        } else if (err.error != MODBUS_ERROR_ADDRESS) {
            ESP_LOGW(TAG, "Invalid request with source %i and error %i", err.source, err.error);
            ESP_LOG_BUFFER_HEX(TAG, buffer, len);
        }
    }

    if (is_expired(timestamp, get_millis(), EASYCONNECT_HEARTBEAT_TIMEOUT)) {
        if (model_get_missing_heartbeat(context->arg) == 0) {
            model_set_missing_heartbeat(context->arg, 1);
        }
    }
}


ModbusError register_callback(const ModbusSlave *status, const ModbusRegisterCallbackArgs *args,
                              ModbusRegisterCallbackResult *result) {

    easyconnect_interface_t *ctx = modbusSlaveGetUserPointer(status);
    result->value                = 0;

    switch (args->query) {
        // R/W access check
        case MODBUS_REGQ_R_CHECK:
            result->exceptionCode = MODBUS_EXCEP_NONE;
            break;

        case MODBUS_REGQ_W_CHECK:
            result->exceptionCode = MODBUS_EXCEP_NONE;

            switch (args->type) {
                case MODBUS_HOLDING_REGISTER: {
                    switch (args->index) {
                        case EASYCONNECT_HOLDING_REGISTER_ADDRESS:
                        case EASYCONNECT_HOLDING_REGISTER_CLASS:
                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_1:
                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_2:
                        case EASYCONNECT_HOLDING_REGISTER_STATE:
                            break;

                        default:
                            result->exceptionCode = MODBUS_EXCEP_ILLEGAL_FUNCTION;
                            break;
                    }
                    break;
                }
                case MODBUS_INPUT_REGISTER:
                    result->exceptionCode = MODBUS_EXCEP_ILLEGAL_FUNCTION;
                    break;
                case MODBUS_DISCRETE_INPUT:
                    result->exceptionCode = MODBUS_EXCEP_ILLEGAL_FUNCTION;
                    break;
                default:
                    break;
            }
            break;

        // Read register
        case MODBUS_REGQ_R:
            switch (args->type) {
                case MODBUS_HOLDING_REGISTER: {
                    switch (args->index) {
                        case EASYCONNECT_HOLDING_REGISTER_ADDRESS:
                            result->value = ctx->get_address(ctx->arg);
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_FIRMWARE_VERSION:
                            result->value = EASYCONNECT_FIRMWARE_VERSION(APP_CONFIG_FIRMWARE_VERSION_MAJOR,
                                                                         APP_CONFIG_FIRMWARE_VERSION_MINOR,
                                                                         APP_CONFIG_FIRMWARE_VERSION_PATCH);
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_CLASS:
                            result->value = ctx->get_class(ctx->arg);
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_1:
                            result->value = (ctx->get_serial_number(ctx->arg) >> 16) & 0xFFFF;
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_2:
                            result->value = ctx->get_serial_number(ctx->arg) & 0xFFFF;
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_ALARMS:
                            result->value =
                                (safety_signal_ok(ctx->arg) == 0) | ((safety_pressure_ok(ctx->arg) == 0) << 1);
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_STATE:
                            result->value = sensors_get_errors();
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_LOGS_COUNTER:
                            result->value = 0;
                            break;

                        case EASYCONNECT_HOLDING_REGISTER_LOGS ... EASYCONNECT_HOLDING_REGISTER_MESSAGE_1 - 1: {
                            result->value = 0;
                            break;
                        }

                        case HOLDING_REGISTER_MINIMUM_PRESSURE_MESSAGE ... HOLDING_REGISTER_MAXIMUM_PRESSURE_MESSAGE -
                            1: {
                            char msg[EASYCONNECT_MESSAGE_SIZE + 1] = {0};
                            model_get_minimum_pressure_message(ctx->arg, msg);
                            size_t i      = args->index - HOLDING_REGISTER_MINIMUM_PRESSURE_MESSAGE;
                            result->value = msg[i * 2] << 8 | msg[i * 2 + 1];
                            break;
                        }

                        case HOLDING_REGISTER_MAXIMUM_PRESSURE_MESSAGE ... HOLDING_REGISTER_MAXIMUM_PRESSURE_MESSAGE +
                            EASYCONNECT_MESSAGE_NUM_REGISTERS - 1: {
                            char msg[EASYCONNECT_MESSAGE_SIZE + 1] = {0};
                            model_get_maximum_pressure_message(ctx->arg, msg);
                            size_t i      = args->index - HOLDING_REGISTER_MAXIMUM_PRESSURE_MESSAGE;
                            result->value = msg[i * 2] << 8 | msg[i * 2 + 1];
                            break;
                        }

                        case HOLDING_REGISTER_PRESSURE:
                            result->value = model_get_pressure(ctx->arg);
                            break;

                        case HOLDING_REGISTER_TEMPERATURE:
                            result->value = model_get_temperature(ctx->arg);
                            break;

                        case HOLDING_REGISTER_HUMIDITY:
                            result->value = model_get_humidity(ctx->arg);
                            break;
                    }
                    break;
                }
                case MODBUS_INPUT_REGISTER:
                    break;
                case MODBUS_COIL:
                    result->value = digout_get();
                    break;
                case MODBUS_DISCRETE_INPUT:
                    result->value = digin_get(args->index);
                    break;
            }
            break;

        // Write register
        case MODBUS_REGQ_W:
            switch (args->type) {
                case MODBUS_HOLDING_REGISTER: {
                    switch (args->index) {
                        case EASYCONNECT_HOLDING_REGISTER_ADDRESS:
                            ctx->save_address(ctx->arg, args->value);
                            break;
                        case EASYCONNECT_HOLDING_REGISTER_CLASS:
                            ctx->save_class(ctx->arg, args->value);
                            break;
                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_1: {
                            uint32_t current_serial_number = ctx->get_serial_number(ctx->arg);
                            ctx->save_serial_number(ctx->arg, (args->value << 16) | (current_serial_number & 0xFFFF));
                            break;
                        }
                        case EASYCONNECT_HOLDING_REGISTER_SERIAL_NUMBER_2: {
                            uint32_t current_serial_number = ctx->get_serial_number(ctx->arg);
                            ctx->save_serial_number(ctx->arg, args->value | (current_serial_number & 0xFFFF0000));
                            break;
                        }
                    }
                    break;
                }

                case MODBUS_COIL:
                    break;

                case MODBUS_INPUT_REGISTER:
                    break;

                default:
                    break;
            }
            break;
    }

    // Always return MODBUS_OK
    return MODBUS_OK;
}


static ModbusError exception_callback(const ModbusSlave *minion, uint8_t function, ModbusExceptionCode code) {
    ESP_LOGI(TAG, "Slave reports an exception %d (function %d)\n", code, function);
    // Always return MODBUS_OK
    return MODBUS_OK;
}


static LIGHTMODBUS_RET_ERROR initialization_function(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                                     uint8_t requestLength) {
    return MODBUS_NO_ERROR();
}


static LIGHTMODBUS_RET_ERROR heartbeat_received(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                                uint8_t requestLength) {
    easyconnect_interface_t *ctx = modbusSlaveGetUserPointer(minion);
    ESP_LOGI(TAG, "Heartbeat");

    timestamp = get_millis();
    model_set_missing_heartbeat(ctx->arg, 0);
    return MODBUS_NO_ERROR();
}


static LIGHTMODBUS_RET_ERROR set_datetime(ModbusSlave *minion, uint8_t function, const uint8_t *requestPDU,
                                          uint8_t requestLength) {
    // Check request length
    if (requestLength < 8) {
        return modbusBuildException(minion, function, MODBUS_EXCEP_ILLEGAL_VALUE);
    }

    uint64_t timestamp = 0;
    deserialize_uint64_be(&timestamp, (uint8_t *)requestPDU);

    struct timeval timeval = {0};
    timeval.tv_sec         = timestamp;

    settimeofday(&timeval, NULL);

    return MODBUS_NO_ERROR();
}
