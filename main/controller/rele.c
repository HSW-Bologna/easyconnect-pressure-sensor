#include "peripherals/digout.h"
#include "safety.h"


int rele_update(uint8_t value) {
    if (safety_ok()) {
        digout_update(DIGOUT_RELE, value);
        digout_update(DIGOUT_LIGHT_SIGNAL, !value);
        return 0;
    } else {
        if (!value) {
            digout_update(DIGOUT_RELE, 0);
        }
        return -1;
    }
}


uint8_t rele_is_on(void) {
    return digout_get();
}