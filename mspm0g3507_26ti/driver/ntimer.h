#ifndef __NTIME_H
#define __NTIME_H



void timer_irq_config(void);
void Timer_Gate_Set(uint8_t enable);
uint8_t Timer_Gate_GetRequested(void);
uint8_t Timer_Gate_GetOutputLatch(void);
uint8_t Timer_Gate_GetOutputEnabled(void);
uint8_t Timer_Gate_GetPinLevel(void);


void Reserved_PWM1_Output(uint16_t us);
void Reserved_PWM2_Output(uint16_t us);
void Reserved_PWM3_Output(uint16_t us);
void Reserved_PWM4_Output(uint16_t us);
void Reserved_PWM5_Output(uint16_t us);
void Reserved_PWM6_Output(uint16_t us);
void Reserved_PWM7_Output(uint16_t us);
void Reserved_PWM8_Output(uint16_t us);


#endif



