#include "approval.h"
#include "peripherals/digout.h"


void approval_on(void) {
    digout_update(DIGOUT_LIGHT_SIGNAL, 0);
    digout_update(DIGOUT_RELE, 1);
}


void approval_off(void) {
    digout_update(DIGOUT_LIGHT_SIGNAL, 1);
    digout_update(DIGOUT_RELE, 0);
}
