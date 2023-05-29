#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "peripherals/i2c_devices.h"
#include "i2c_devices/temperature/MS5837/ms5837.h"
#include "i2c_devices/temperature/SHTC3/shtc3.h"
#include "config/app_config.h"


#define NUM_SAMPLES_PRESSURE 200
#define NUM_SAMPLES_SHTC3    50


static void temperature_task(void *args);
static void pressure_task(void *args);


static const char *TAG = "Sensors";


static SemaphoreHandle_t sem                                          = NULL;
static double            temperatures[NUM_SAMPLES_SHTC3]              = {0};
static double            humidities[NUM_SAMPLES_SHTC3]                = {0};
static uint32_t          temperature_adc_buffer[NUM_SAMPLES_PRESSURE] = {0};
static uint32_t          pressure_adc_buffer[NUM_SAMPLES_PRESSURE]    = {0};
static size_t            ms5837_sample_index                          = 0;
static uint8_t           ms5837_full_circle                           = 0;
static ms5837_prom_t     ms5837_data;
static size_t            shtc3_sample_index = 0;
static uint8_t           shtc3_full_circle  = 0;


void sensors_init(uint8_t pressure, uint8_t temperature_humidity) {
    static StaticSemaphore_t semaphore_buffer;
    sem = xSemaphoreCreateMutexStatic(&semaphore_buffer);

    if (pressure) {
        static StaticTask_t static_task;
        static StackType_t  task_stack[APP_CONFIG_BASE_TASK_STACK_SIZE * 3];
        xTaskCreateStatic(pressure_task, TAG, sizeof(task_stack) / sizeof(StackType_t), NULL, 5, task_stack,
                          &static_task);
    }


    if (temperature_humidity) {
        static StaticTask_t static_task;
        static StackType_t  task_stack[APP_CONFIG_BASE_TASK_STACK_SIZE * 3];
        xTaskCreateStatic(temperature_task, TAG, sizeof(task_stack) / sizeof(StackType_t), NULL, 1, task_stack,
                          &static_task);
    }
}


void sensors_read(double *temperature, double *pressure, double *humidity) {
    xSemaphoreTake(sem, portMAX_DELAY);
    size_t ms5837_total = ms5837_full_circle ? NUM_SAMPLES_PRESSURE : ms5837_sample_index;

    uint64_t temperature_sum = 0;
    uint64_t pressure_sum    = 0;

    for (size_t i = 0; i < ms5837_total; i++) {
        temperature_sum += temperature_adc_buffer[i];
        pressure_sum += pressure_adc_buffer[i];
    }

    size_t shtc3_total = shtc3_full_circle ? NUM_SAMPLES_SHTC3 : shtc3_sample_index;

    double shtc3_temperature_sum = 0;
    double humidity_sum          = 0;

    for (size_t i = 0; i < shtc3_total; i++) {
        shtc3_temperature_sum += temperatures[i];
        humidity_sum += humidities[i];
    }

    xSemaphoreGive(sem);

    if (ms5837_total == 0) {
        *pressure = 0;
    } else {
        ms5837_calculate(ms5837_data, temperature_sum / ms5837_total, pressure_sum / ms5837_total, NULL, pressure);
    }

    if (shtc3_total == 0) {
        *temperature = 0;
        *humidity    = 0;
    } else {
        *temperature = shtc3_temperature_sum / ((double)shtc3_total);
        *humidity    = humidity_sum / ((double)shtc3_total);
    }
}


static void temperature_task(void *args) {
    (void)args;
    shtc3_wakeup(shtc3_driver);

    for (;;) {
        int16_t temperature = 0;
        int16_t humidity    = 0;

        if (shtc3_start_temperature_humidity_measurement(shtc3_driver) == 0) {
            vTaskDelay(pdMS_TO_TICKS(SHTC3_NORMAL_MEASUREMENT_PERIOD_MS));

            if (shtc3_read_temperature_humidity_measurement(shtc3_driver, &temperature, &humidity) == 0) {
                xSemaphoreTake(sem, portMAX_DELAY);
                temperatures[shtc3_sample_index] = (double)temperature;
                humidities[shtc3_sample_index]   = (double)humidity;
                if (shtc3_sample_index == NUM_SAMPLES_SHTC3 - 1) {
                    shtc3_full_circle = 1;
                }
                shtc3_sample_index = (shtc3_sample_index + 1) % NUM_SAMPLES_SHTC3;
                xSemaphoreGive(sem);
            } else {
                ESP_LOGD(TAG, "Error in reading temperature measurement");
            }
        } else {
            ESP_LOGD(TAG, "Error in starting temperature measurement");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}


static void pressure_task(void *args) {
    (void)args;

    uint16_t retry_counter = 0;

    ms5837_init(press_driver, &ms5837_data);

    for (;;) {
        uint32_t temperature_adc = 0, pressure_adc = 0;

        // These functions are blocking
        int res = ms5837_read_temperature_adc(press_driver, MS5837_OSR_8192, &temperature_adc);
        res     = res || ms5837_read_pressure_adc(press_driver, MS5837_OSR_8192, &pressure_adc);

        if (res) {
            ESP_LOGW(TAG, "Error reading sensor: %i", res);
            if ((retry_counter++ % 10) == 0) {
                ms5837_init(press_driver, &ms5837_data);
            }
        } else {
            retry_counter = 0;

            xSemaphoreTake(sem, portMAX_DELAY);
            temperature_adc_buffer[ms5837_sample_index] = temperature_adc;
            pressure_adc_buffer[ms5837_sample_index]    = pressure_adc;
            ms5837_sample_index++;
            if (ms5837_sample_index >= NUM_SAMPLES_PRESSURE) {
                ms5837_full_circle  = 1;
                ms5837_sample_index = 0;
            }
            xSemaphoreGive(sem);
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }

    vTaskDelete(NULL);
}