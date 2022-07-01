#ifndef HEARTBEAT_H_INCLUDED
#define HEARTBEAT_H_INCLUDED


#include <stdlib.h>


typedef enum {
    HEARTBEAT_STATE_OK,
    HEARTBEAT_STATE_KO,
} heartbeat_state_t;


void heartbeat_init(void);
void heartbeat_set_state(heartbeat_state_t state);


#endif