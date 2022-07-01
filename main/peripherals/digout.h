#ifndef DIGOUT_H_INCLUDED
#define DIGOUT_H_INCLUDED

#include <string.h>
#include <stdint.h>


typedef enum {
    DIGOUT_RELE = 0,
    DIGOUT_LIGHT_SIGNAL,
} digout_t;


void    digout_init(void);
void    digout_update(digout_t digout, uint8_t val);
uint8_t digout_get(void);

// #define toggle_digout_port(rele_t rele)  update_digout_port(out, !get_digout_port(out))
#define set_digout(rele)   rele_update(rele, 1)
#define clear_digout(rele) rele_update(rele, 0)

#endif