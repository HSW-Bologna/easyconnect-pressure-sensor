#include "minion.h"
#include "config/app_config.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/projdefs.h"
#include "hardwareprofile.h"
#include "lightmodbus/base.h"
#include "lightmodbus/lightmodbus.h"
#include "lightmodbus/master.h"
#include "lightmodbus/slave.h"
#include "lightmodbus/slave_func.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "storage.h"
#include "digout.h"
#include "digin.h"


#define ADDRESS_KEY             "indirizzo"
#define SERIAL_NUM_KEY          "numero seriale"
#define MODEL_KEY               "modello"
#define MODBUS_TIMEOUT          10
#define MB_PORTNUM              1
#define SLAVE_ADDR              1
#define SERIAL_NUMBER           2
#define MODEL_NUMBER            0
#define REG_COUNT               3
#define CONFIG_ADDRESS_FUNCTION 64
#define NETWORK_INIZIALIZATION  65
#define RANDOM_SERIAL_NUMBER    66
#define SET_CLASS_OUTPUT        70
// Timeout threshold for UART = number of symbols (~10 tics) with unchanged
// state on receive pin
#define ECHO_READ_TOUT (3)     // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks


typedef struct {
    uint16_t address;
    uint16_t model;
    uint16_t serial_n;
} minion_context_t;


static const char      *TAG = "Minion";
static minion_context_t context;
ModbusSlave             slave;

ModbusError myRegisterCallback(const ModbusSlave *status, const ModbusRegisterCallbackArgs *args,
                               ModbusRegisterCallbackResult *result);

ModbusError                  myExceptionCallback(const ModbusSlave *slave, uint8_t function, ModbusExceptionCode code);
static LIGHTMODBUS_RET_ERROR setAddressFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                uint8_t requestLength);
static LIGHTMODBUS_RET_ERROR sendAddressFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                 uint8_t requestLength);
static LIGHTMODBUS_RET_ERROR inizializationFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                    uint8_t requestLength);
static LIGHTMODBUS_RET_ERROR setClassOutput(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                            uint8_t requestLength);

ModbusSlaveFunctionHandler custom_functions[] = {
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

    {CONFIG_ADDRESS_FUNCTION, setAddressFunction},
    {RANDOM_SERIAL_NUMBER, sendAddressFunction},
    {NETWORK_INIZIALIZATION, inizializationFunction},
    {SET_CLASS_OUTPUT, setClassOutput},

    // Guard - prevents 0 array size
    {0, NULL}};


void minion_init(void) {
    context.address  = SLAVE_ADDR;
    context.serial_n = SERIAL_NUMBER;
    context.model    = MODEL_NUMBER;
    ModbusErrorInfo err;
    err = modbusSlaveInit(&slave,
                          myRegisterCallback,         // Callback for register operations
                          myExceptionCallback,        // Callback for handling slave exceptions (optional)
                          modbusDefaultAllocator,     // Memory allocator for allocating responses
                          custom_functions,           // Set of supported functions
                          13                          // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusSlaveInit() failed");

    modbusSlaveSetUserPointer(&slave, &context);

    uart_config_t uart_config = {
        .baud_rate           = 115200,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(SLAVE_ADDR, &uart_config));

    uart_set_pin(MB_PORTNUM, MB_UART_TXD, MB_UART_RXD, MB_DERE, -1);
    ESP_ERROR_CHECK(uart_driver_install(MB_PORTNUM, 512, 512, 10, NULL, 0));
    ESP_ERROR_CHECK(uart_set_mode(MB_PORTNUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_ERROR_CHECK(uart_set_rx_timeout(MB_PORTNUM, ECHO_READ_TOUT));

    if (load_uint16_option(&context.address, ADDRESS_KEY)) {
        context.address = SLAVE_ADDR;
    }
    ESP_LOGI(TAG, "Address: %02X", context.address);

    if (load_uint16_option(&context.serial_n, SERIAL_NUM_KEY)) {
        context.serial_n = SERIAL_NUMBER;
    }
    ESP_LOGI(TAG, "Serial number: %02X", context.serial_n);

    if (load_uint16_option(&context.model, MODEL_KEY)) {
        context.model = MODEL_NUMBER;
    }
    ESP_LOGI(TAG, "Model: %02X", context.model);

    uint8_t mac_address[6] = {0};
    esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
    unsigned int mac = mac_address[2] | mac_address[3] << 8 | mac_address[4] << 16 | mac_address[5] << 24;
    ESP_LOGI(TAG, "MAC Address: %i", mac);
    srand(mac);
}

void minion_manage(void) {
    uint8_t buffer[256] = {0};
    int     len         = uart_read_bytes(MB_PORTNUM, buffer, 256, pdMS_TO_TICKS(MODBUS_TIMEOUT));
    if (len > 0) {
        ModbusErrorInfo err;
        err = modbusParseRequestRTU(&slave, context.address, buffer, len);

        if (modbusIsOk(err)) {
            size_t rlen = modbusSlaveGetResponseLength(&slave);
            if (rlen > 0) {
                uart_write_bytes(MB_PORTNUM, modbusSlaveGetResponse(&slave), rlen);
            } else {
                ESP_LOGI(TAG, "Empty response");
            }
        } else if (err.error != MODBUS_ERROR_ADDRESS) {
            ESP_LOGW(TAG, "Invalid request with source %i and error %i", err.source, err.error);
            ESP_LOG_BUFFER_HEX(TAG, buffer, len);
        }
    }
}


ModbusError myRegisterCallback(const ModbusSlave *status, const ModbusRegisterCallbackArgs *args,
                               ModbusRegisterCallbackResult *result) {

    minion_context_t *ctx = modbusSlaveGetUserPointer(status);

    printf("%i %i %i %i\n", args->query, args->type, args->index, args->value);
    switch (args->query) {
        // R/W access check
        case MODBUS_REGQ_R_CHECK:
        case MODBUS_REGQ_W_CHECK:
            // If result->exceptionCode of a read/write access query is not
            // MODBUS_EXCEP_NONE, an exception is reported by the slave. If
            // result->exceptionCode is not set, the behavior is undefined.
            if (args->type == MODBUS_INPUT_REGISTER) {
                result->exceptionCode = MODBUS_EXCEP_ILLEGAL_FUNCTION;
            }
            result->exceptionCode = args->index < REG_COUNT ? MODBUS_EXCEP_NONE : MODBUS_EXCEP_ILLEGAL_ADDRESS;
            break;

        // Read register
        case MODBUS_REGQ_R:
            switch (args->type) {
                case MODBUS_HOLDING_REGISTER: {
                    uint16_t registers[] = {ctx->address, ctx->model, ctx->serial_n};
                    result->value        = registers[args->index];
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
                    uint16_t *registers[]   = {&ctx->address, &ctx->model, &ctx->serial_n};
                    *registers[args->index] = args->value;
                    char *keys[]            = {ADDRESS_KEY, MODEL_KEY, SERIAL_NUM_KEY};
                    save_uint16_option(registers[args->index], keys[args->index]);
                    break;
                }
                case MODBUS_COIL:
                    digout_rele_update(args->value);
                    ESP_LOGI(TAG, "update rele value %i", args->value);
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

ModbusError myExceptionCallback(const ModbusSlave *slave, uint8_t function, ModbusExceptionCode code) {
    printf("Slave reports an exception %d (function %d)\n", code, function);

    // Always return MODBUS_OK
    return MODBUS_OK;
}

static LIGHTMODBUS_RET_ERROR setAddressFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                uint8_t requestLength) {
    // Check request length
    if (requestLength < 1) {
        return modbusBuildException(slave, function, MODBUS_EXCEP_ILLEGAL_VALUE);
    }
    // Check for the 'invalid' case
    if (requestPDU[1] == 0) {
        return modbusBuildException(slave, function, MODBUS_EXCEP_ILLEGAL_VALUE);
    }

    uint16_t serial_number;
    if (requestLength == 4) {
        serial_number = (requestPDU[2] << 8) | requestPDU[3];
    }

    minion_context_t *ctx = modbusSlaveGetUserPointer(slave);
    if (requestLength == 2 || (requestLength == 4 && serial_number == ctx->serial_n)) {
        ctx->address = requestPDU[1];
        save_uint16_option(&ctx->address, ADDRESS_KEY);
    }

    return MODBUS_NO_ERROR();
}


static LIGHTMODBUS_RET_ERROR sendAddressFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                 uint8_t requestLength) {
    minion_context_t *ctx = modbusSlaveGetUserPointer(slave);

    // Check request length
    if (requestLength < 3) {
        return modbusBuildException(slave, function, MODBUS_EXCEP_ILLEGAL_VALUE);
    }

    uint16_t delay        = requestPDU[1] << 8 | requestPDU[2];
    int      random_delay = rand() % (delay * 1000);

    ESP_LOGI(TAG, "Random delay: %i", random_delay);
    // vTaskDelay(random_delay);


    ESP_LOGI(TAG, "Serial number: %i", ctx->serial_n);

    // ---- RESPONSE ----
    uint8_t response[4];
    response[0]  = (ctx->serial_n >> 8) & 0xFF;
    response[1]  = ctx->serial_n & 0xFF;
    uint16_t crc = modbusCRC(response, 2);
    ESP_LOGI(TAG, "CRC: %i", crc);
    response[2] = (crc >> 8) & 0xFF;
    response[3] = crc & 0xFF;

    vTaskDelay(pdMS_TO_TICKS(random_delay));
    uart_write_bytes(MB_PORTNUM, response, 4);
    vTaskDelay(pdMS_TO_TICKS(delay * 1000 - random_delay));

    return MODBUS_NO_ERROR();
}

static LIGHTMODBUS_RET_ERROR inizializationFunction(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                                    uint8_t requestLength) {
    digout_rele_update(0);
    ESP_LOGI(TAG, "rele off");

    return MODBUS_NO_ERROR();
}

static LIGHTMODBUS_RET_ERROR setClassOutput(ModbusSlave *slave, uint8_t function, const uint8_t *requestPDU,
                                            uint8_t requestLength) {
    // Check request length
    if (requestLength < 3)
        return modbusBuildException(slave, function, MODBUS_EXCEP_ILLEGAL_VALUE);

    minion_context_t *ctx = modbusSlaveGetUserPointer(slave);
    uint16_t class        = requestPDU[1] << 8 | requestPDU[2];
    ESP_LOGI(TAG, "class & model %i, %i", class, ctx->model);
    if (class == ctx->model) {
        digout_rele_update(requestPDU[3]);
        ESP_LOGI(TAG, "update rele value %i", requestPDU[3]);
    }

    return MODBUS_NO_ERROR();
}