#ifndef HEARTBEAT_H_INCLUDED
#define HEARTBEAT_H_INCLUDED


#include <stdlib.h>


void heartbeat_init(size_t period_ms);
void heartbeat_resume(void);
void heartbeat_stop(void);


#endif