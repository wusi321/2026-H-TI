#include "ti_msp_dl_config.h"
#include "system.h"



volatile uint32_t sysTickUptime = 0;
void SysTick_Handler(void)
{
		sysTickUptime++;
}

uint32_t  irq_poriority[16];
void ncontroller_set_priority(void)
{
	__NVIC_SetPriority(SysTick_IRQn,0);

	//
	irq_poriority[0]=__NVIC_GetPriority(SysTick_IRQn);
	irq_poriority[1]=__NVIC_GetPriority(GPIOB_INT_IRQn);
	irq_poriority[2]=__NVIC_GetPriority(GPIOA_INT_IRQn);
	irq_poriority[3]=__NVIC_GetPriority(TIMG0_INT_IRQn);
	irq_poriority[4]=__NVIC_GetPriority(TIMG6_INT_IRQn);
	irq_poriority[5]=__NVIC_GetPriority(TIMG8_INT_IRQn);
	irq_poriority[6]=__NVIC_GetPriority(TIMG12_INT_IRQn);
	irq_poriority[7]=__NVIC_GetPriority(UART0_INT_IRQn);
	irq_poriority[8]=__NVIC_GetPriority(UART1_INT_IRQn);
	irq_poriority[9]=__NVIC_GetPriority(UART2_INT_IRQn);
	irq_poriority[10]=__NVIC_GetPriority(UART3_INT_IRQn);
	irq_poriority[11]=__NVIC_GetPriority(ADC0_INT_IRQn);
}

//返回 us
uint32_t micros(void)
{
  uint32_t systick_period = CPUCLK_FREQ / 1000U;
  return sysTickUptime * 1000 + (1000 * (systick_period - SysTick->VAL)) / systick_period;
}

//返回 ms
uint32_t millis(void) {
	return micros() / 1000UL;
}

//延时us
void delayMicroseconds(uint32_t us) {
  uint32_t start = micros();
  while((int32_t)(micros() - start) < us) {
    // Do nothing
  };
}

void delay(uint32_t ms) {
  delayMicroseconds(ms * 1000UL);
}


void delay_ms(uint32_t x)
{
  delay(x);
}

void delay_us(uint32_t x)
{
  delayMicroseconds(x);
}

void Delay_Ms(uint32_t time)  //延时函数  
{   
	delay_ms(time);
}  

void Delay_Us(uint32_t time)  //延时函数  
{   
	delay_us(time);
}  


void get_systime(systime *sys)
{
  sys->last_time=sys->current_time;
  sys->current_time=micros()/1000.0f;//单位ms
  sys->period=sys->current_time-sys->last_time;
  sys->period_int=(uint16_t)(sys->period+0.5f);//四舍五入
}



float get_systime_ms(void)
{
  return millis();//单位ms
}

uint32_t get_systick_ms(void)
{
  return (uint32_t)(sysTickUptime);//单位ms
}



volatile uint32_t nmi_cnt = 0;
void NMI_Handler(void)
{
	nmi_cnt++;
}


volatile uint32_t hf_cnt = 0;
void HardFault_Handler(void)
{
	hf_cnt++;
}




