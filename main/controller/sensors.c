#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "peripherals/i2c_devices.h"
#include "i2c_devices/temperature/MS5837/ms5837.h"


#define NUM_SAMPLES 10


static void read_timer(TimerHandle_t timer);


static const char *TAG = "Sensors";


static SemaphoreHandle_t sem                                 = NULL;
static uint32_t          temperature_adc_buffer[NUM_SAMPLES] = {0};
static uint32_t          pressure_adc_buffer[NUM_SAMPLES]    = {0};
static size_t            sample_index                        = 0;
static uint8_t           full_circle                         = 0;
static ms5837_prom_t     ms5837_data;


void sensors_init(void) {
    ms5837_init(press_driver, &ms5837_data);

    static StaticSemaphore_t semaphore_buffer;
    sem = xSemaphoreCreateMutexStatic(&semaphore_buffer);

    static StaticTimer_t timer_buffer;
    TimerHandle_t        timer = xTimerCreateStatic(TAG, pdMS_TO_TICKS(100), 1, NULL, read_timer, &timer_buffer);
    xTimerStart(timer, portMAX_DELAY);
}


void sensors_read(double *temperature, double *pressure) {
    size_t total = full_circle ? NUM_SAMPLES : sample_index;

    uint64_t temperature_sum = 0;
    uint64_t pressure_sum    = 0;

    if (total == 0) {
        *temperature = 0;
        *pressure    = 0;
        return;
    }

    xSemaphoreTake(sem, portMAX_DELAY);
    for (size_t i = 0; i < total; i++) {
        temperature_sum += temperature_adc_buffer[i];
        pressure_sum += pressure_adc_buffer[i];
    }
    xSemaphoreGive(sem);

    ms5837_calculate(ms5837_data, temperature_sum / total, pressure_sum / total, temperature, pressure);
}


static void read_timer(TimerHandle_t timer) {
    static uint16_t retry_counter   = 0;
    uint32_t        temperature_adc = 0, pressure_adc = 0;
    int             res = ms5837_read_temperature_adc(press_driver, MS5837_OSR_8192, &temperature_adc);
    res                 = res || ms5837_read_pressure_adc(press_driver, MS5837_OSR_8192, &pressure_adc);

    if (res) {
        ESP_LOGW(TAG, "Error reading sensor: %i", res);
        if ((retry_counter++ % 10) == 0) {
            ms5837_init(press_driver, &ms5837_data);
        }
    } else {
        retry_counter = 0;

        xSemaphoreTake(sem, portMAX_DELAY);
        temperature_adc_buffer[sample_index] = temperature_adc;
        pressure_adc_buffer[sample_index]    = pressure_adc;
        sample_index++;
        if (sample_index >= NUM_SAMPLES) {
            full_circle  = 1;
            sample_index = 0;
        }
        xSemaphoreGive(sem);
    }
}