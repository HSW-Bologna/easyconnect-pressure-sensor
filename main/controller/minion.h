#ifndef MINION_H_INCLUDED
#define MINION_H_INCLUDED


#include "model/model.h"
#include "easyconnect.h"


void minion_init(easyconnect_interface_t *context);
void minion_manage(void);

#endif