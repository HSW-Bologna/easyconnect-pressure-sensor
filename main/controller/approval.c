#include "approval.h"
#include "peripherals/digout.h"


void approval_on(void) {
    digout_update(DIGOUT_APPROVAL, 1);
}


void approval_off(void) {
    digout_update(DIGOUT_APPROVAL, 0);
}
