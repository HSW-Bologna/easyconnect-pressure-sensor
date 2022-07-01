#include "model.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "easyconnect.h"


void model_init(model_t *pmodel) {
    pmodel->sem = xSemaphoreCreateMutexStatic(&pmodel->semaphore_buffer);

    pmodel->address         = EASYCONNECT_DEFAULT_MINION_ADDRESS;
    pmodel->serial_number   = EASYCONNECT_DEFAULT_MINION_SERIAL_NUMBER;
    pmodel->class           = EASYCONNECT_DEFAULT_MINION_MODEL;
    pmodel->feedback_level  = EASYCONNECT_DEFAULT_FEEDBACK_LEVEL;
    pmodel->output_attempts = EASYCONNECT_DEFAULT_ACTIVATE_ATTEMPTS;
    pmodel->feedback_delay  = EASYCONNECT_DEFAULT_FEEDBACK_DELAY;
}