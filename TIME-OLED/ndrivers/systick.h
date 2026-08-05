#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "ti_msp_dl_config.h"

/* º¯ÊýÉùÃ÷ */
void systick_init(void);
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);

#endif /* __BSP_SYSTICK_H */