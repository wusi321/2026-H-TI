#ifndef __NUART_H__
#define __NUART_H__

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

// 各个串口接收缓冲区的最大长度
#define UART_RX_BUF_SIZE   64

/* --- 函数接口声明 --- */
void usart_irq_config(void);
void UART_SendByte(UART_Regs *port, uint8_t data);
void UART_SendBytes(UART_Regs *port, uint8_t *ubuf, uint32_t len);
void UART_SendString(UART_Regs *port, char *str);

#endif /* __NUART_H__ */