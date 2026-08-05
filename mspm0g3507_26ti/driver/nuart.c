#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "datatype.h"
#include "system.h"
#include "motor_control.h"
#include "sensor.h"
#include "nclink.h"
#include "us100.h"
#include "vision.h"
#include "ball_balance.h"
#include "nuart.h"

#define UART1_ERROR_INTERRUPT_MASK                                      \
    (DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |                             \
     DL_UART_MAIN_INTERRUPT_BREAK_ERROR |                               \
     DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |                             \
     DL_UART_MAIN_INTERRUPT_PARITY_ERROR |                              \
     DL_UART_MAIN_INTERRUPT_NOISE_ERROR)

volatile uint32_t g_uart1_overrun_error_count;
volatile uint32_t g_uart1_break_error_count;
volatile uint32_t g_uart1_framing_error_count;
volatile uint32_t g_uart1_parity_error_count;
volatile uint32_t g_uart1_noise_error_count;

void usart_irq_config(void)
{
  NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
  NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
  NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
  NVIC_ClearPendingIRQ(UART_3_INST_INT_IRQN);
	DL_UART_clearInterruptStatus(UART_0_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
	DL_UART_Main_enableInterrupt(UART_1_INST, UART1_ERROR_INTERRUPT_MASK);
	DL_UART_clearInterruptStatus(UART_1_INST,
		DL_UART_INTERRUPT_RX | UART1_ERROR_INTERRUPT_MASK);//清除中断标志位
	DL_UART_clearInterruptStatus(UART_2_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
	DL_UART_clearInterruptStatus(UART_3_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
	NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_3_INST_INT_IRQN);
	
	NVIC_SetPriority(SysTick_IRQn,0);
}

void UART_0_INST_IRQHandler(void)
{
  if(DL_UART_getEnabledInterruptStatus(UART_0_INST,DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
  {
		uint8_t ch = DL_UART_receiveData(UART_0_INST);
		NCLink_Data_Prase_Prepare_Lite(ch);
		DL_UART_clearInterruptStatus(UART_0_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
  }
}


void UART_1_INST_IRQHandler(void)
{
  uint32_t error_status = DL_UART_Main_getRawInterruptStatus(
      UART_1_INST, UART1_ERROR_INTERRUPT_MASK);

  if (error_status != 0U) {
    if ((error_status & DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR) != 0U) {
      g_uart1_overrun_error_count++;
    }
    if ((error_status & DL_UART_MAIN_INTERRUPT_BREAK_ERROR) != 0U) {
      g_uart1_break_error_count++;
    }
    if ((error_status & DL_UART_MAIN_INTERRUPT_FRAMING_ERROR) != 0U) {
      g_uart1_framing_error_count++;
    }
    if ((error_status & DL_UART_MAIN_INTERRUPT_PARITY_ERROR) != 0U) {
      g_uart1_parity_error_count++;
    }
    if ((error_status & DL_UART_MAIN_INTERRUPT_NOISE_ERROR) != 0U) {
      g_uart1_noise_error_count++;
    }

    BallBalance_UART1_ResetParser();
    DL_UART_Main_clearInterruptStatus(UART_1_INST, error_status);
  }

  while (!DL_UART_Main_isRXFIFOEmpty(UART_1_INST)) {
    BallBalance_UART1_RxByte(DL_UART_Main_receiveData(UART_1_INST));
  }
}

systime uart2_dma_t;
void UART_2_INST_IRQHandler(void)
{
  if(DL_UART_getEnabledInterruptStatus(UART_2_INST,DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
  {
		uint8_t ch = DL_UART_receiveData(UART_2_INST);
		bluetooth_app_prase(ch);
		DL_UART_clearInterruptStatus(UART_2_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
  }

	switch (DL_UART_Main_getPendingInterrupt(UART2)) 
	{
		case DL_UART_MAIN_IIDX_DMA_DONE_TX:
			get_systime(&uart2_dma_t);
		break;
		default:break;
	}
}

void UART_3_INST_IRQHandler(void)
{
  if(DL_UART_getEnabledInterruptStatus(UART_3_INST,DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
  {
		uint8_t ch = DL_UART_receiveData(UART_3_INST);
		if(com3_rx_cnt>=2) com3_rx_cnt=0;
		com3_rx_buf[com3_rx_cnt++]=ch;
		DL_UART_clearInterruptStatus(UART_3_INST,DL_UART_INTERRUPT_RX);//清除中断标志位
  }
}


void usart0_send_string(uint8_t *ucstr)
{
  while(ucstr && *ucstr)
  {
    //DL_UART_Main_transmitData(UART_0_INST, *ucstr++);
    DL_UART_Main_transmitDataBlocking(UART_0_INST, *ucstr++);
  }
}


void usart0_send_bytes(unsigned char *buf, int len)
{
  while(len--)
  {
    DL_UART_Main_transmitDataBlocking(UART_0_INST, *buf);//DL_UART_Main_transmitData(UART_0_INST, *buf);
    buf++;
  }
}


int fputc(int ch, FILE *f)
{
  DL_UART_Main_transmitDataBlocking(UART_0_INST, ch);
  return ch;
}




void usart1_send_bytes(unsigned char *buf, int len)
{
  while(len--)
  {
    DL_UART_Main_transmitDataBlocking(UART_1_INST, *buf);
    buf++;
  }
}

/***************************************
函数名:	void UART_SendBytes(uint32_t port,uint8_t *ubuf, uint32_t len)
说明: 发送N个字节长度的数据
入口:	uint32_t port-串口号
			uint8_t *ubuf-待发生数据地址
			uint32_t len-待发送数据长度
出口:	无
备注:	无
作者:	无名创新
***************************************/
void UART_SendBytes(UART_Regs *port,uint8_t *ubuf, uint32_t len)
{
  while(len--)
  {
    DL_UART_Main_transmitDataBlocking(port, *ubuf);
    ubuf++;
  }
}

/***************************************
函数名:	void UART_SendByte(uint32_t port,uint8_t data)
说明: 发送1个字节长度的数据
入口:	uint32_t port-串口号
			uint8_t data-待发生数据
出口:	无
备注:	无
作者:	无名创新
***************************************/
void UART_SendByte(UART_Regs *port,uint8_t data)
{
  DL_UART_Main_transmitDataBlocking(port, data);//DL_UART_Main_transmitData(UART_0_INST, *buf);
}






