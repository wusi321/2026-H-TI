#include "ti_msp_dl_config.h"
#include "system.h"

#include "ntimer.h"



systime timer_t1a,timer_t5a,timer_t6a,timer_t7a;
static volatile uint8_t timer_gate_requested=0U;
extern void maple_duty_200hz(void);
extern void duty_1000hz(void);
extern void duty_100hz(void);
extern void duty_10hz(void);


void timer_irq_config(void)
{
  //pwm
  DL_TimerA_startCounter(PWM_0_INST);
  DL_TimerA_startCounter(PWM_1_INST);
  DL_TimerG_startCounter(PWM_2_INST);
	
  NVIC_EnableIRQ(TIMER_G0_INST_INT_IRQN);
  DL_TimerG_startCounter(TIMER_G0_INST);
  NVIC_EnableIRQ(TIMER_G6_INST_INT_IRQN);
  DL_TimerG_startCounter(TIMER_G6_INST);
	
  NVIC_EnableIRQ(TIMER_G8_INST_INT_IRQN);
  DL_TimerG_startCounter(TIMER_G8_INST);
	
  NVIC_EnableIRQ(TIMER_G12_INST_INT_IRQN);
  DL_TimerG_startCounter(TIMER_G12_INST);
}

void timer_pwm_config(void)
{
  //pwm
  DL_TimerA_startCounter(PWM_0_INST);//A0
  DL_TimerA_startCounter(PWM_1_INST);//A1
  DL_TimerG_startCounter(PWM_2_INST);//G7
}




systime t_g0;
void TIMER_G0_INST_IRQHandler(void)
{
  switch (DL_TimerG_getPendingInterrupt(TIMER_G0_INST))
  {
    case DL_TIMERG_IIDX_ZERO:
    {
      get_systime(&t_g0);
      maple_duty_200hz();
      static uint32_t _cnt = 0; _cnt++;
      if(_cnt % 50 == 0)	DL_GPIO_togglePins(GPIO_RGB_PORT, GPIO_RGB_GREEN_PIN);
    }
    break;

    default:
      break;
  }
}


systime t_g1;
void TIMER_G6_INST_IRQHandler(void)//地面站数据发送中断函数
{
  switch (DL_TimerG_getPendingInterrupt(TIMER_G6_INST))
  {
    case DL_TIMERG_IIDX_ZERO:
    {
      get_systime(&t_g1);
   		duty_100hz();
    }
    break;

    default:
      break;
  }
}

systime t_g8;
void TIMER_G8_INST_IRQHandler(void)//地面站数据发送中断函数
{
  switch (DL_TimerG_getPendingInterrupt(TIMER_G8_INST))
  {
    case DL_TIMERG_IIDX_ZERO:
    {
      get_systime(&t_g8);
			duty_10hz();
    }
    break;
    default:
      break;
  }
}


systime t_g12;
void TIMER_G12_INST_IRQHandler(void)//地面站数据发送中断函数
{
  switch (DL_TimerG_getPendingInterrupt(TIMER_G12_INST))
  {
    case DL_TIMERG_IIDX_ZERO:
    {
      get_systime(&t_g12);
			duty_1000hz();
    }
    break;
    default:
      break;
  }
}



void Reserved_PWM5_Output(uint16_t us)
{
  (void)us;
}

void Timer_Gate_Set(uint8_t enable)
{
  /* Reassert the GPIO mux and output driver on every state request. Task
   * code calls this continuously while timing, so an accidental peripheral
   * reconfiguration cannot leave PA15 latched low. */
  DL_GPIO_initDigitalOutput(PORTA_TIMER_GATE_IOMUX);
  /* Keep the input buffer enabled so DIN reports the physical PA15 level. */
  IOMUX->SECCFG.PINCM[PORTA_TIMER_GATE_IOMUX]|=IOMUX_PINCM_INENA_ENABLE;
  if(enable != 0U)
  {
    DL_GPIO_setPins(PORTA_PORT, PORTA_TIMER_GATE_PIN);
    timer_gate_requested=1U;
  }
  else
  {
    DL_GPIO_clearPins(PORTA_PORT, PORTA_TIMER_GATE_PIN);
    timer_gate_requested=0U;
  }
  DL_GPIO_enableOutput(PORTA_PORT, PORTA_TIMER_GATE_PIN);
}

uint8_t Timer_Gate_GetRequested(void)
{
  return timer_gate_requested;
}

uint8_t Timer_Gate_GetOutputLatch(void)
{
  return (PORTA_PORT->DOUT31_0&PORTA_TIMER_GATE_PIN)!=0U;
}

uint8_t Timer_Gate_GetOutputEnabled(void)
{
  return (PORTA_PORT->DOE31_0&PORTA_TIMER_GATE_PIN)!=0U;
}

uint8_t Timer_Gate_GetPinLevel(void)
{
  return DL_GPIO_readPins(PORTA_PORT,PORTA_TIMER_GATE_PIN)!=0U;
}

void Reserved_PWM6_Output(uint16_t us)
{
  DL_TimerA_setCaptureCompareValue(PWM_1_INST, us, GPIO_PWM_1_C1_IDX);//TIMA1-CH1-PB1
}

void Reserved_PWM7_Output(uint16_t us)
{
  DL_TimerG_setCaptureCompareValue(PWM_2_INST, us, GPIO_PWM_2_C0_IDX);//TIMG7-CH0-PA17
}

void Reserved_PWM8_Output(uint16_t us)
{
  DL_TimerG_setCaptureCompareValue(PWM_2_INST, us, GPIO_PWM_2_C1_IDX);//TIMG7-CH0-PA2
}


