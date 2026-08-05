#include "uart.h"
#include <stdio.h>

// 1. 禁用半主机模式（Semihosting）
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
#endif

// 2. 【核心关键】通过别名机制，解除链接器对 __use_two_region_memory 的死板检索
__asm(".global __ARM_use_no_argv\n\t");

/* 定义标准 C 库所需的 stdout 结构体占位 */
FILE __stdout;

/**
 * @brief 适配标准库的 fputc 重定向，绑定至 UART0
 */
int fputc(int ch, FILE *f)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t)ch);
    DL_UART_clearInterruptStatus(UART_0_INST, DL_UART_INTERRUPT_TX);
    return ch;
}

/* 3. 实现标准库底层的退出与异常处理函数 */
void _sys_exit(int x)
{
    (void)x;
    while(1); 
}

void _ttywrch(int ch)
{
    (void)ch;
}
/**
 * @brief 开启并统一清理所有配置好的串口中断标志
 */
void usart_irq_config(void)
{
    // 清除并使能内核 NVIC 中的中断请求
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_3_INST_INT_IRQN);
    
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_3_INST_INT_IRQN);

    // 清除芯片外设层面的 RX 中断标志位
    DL_UART_clearInterruptStatus(UART_0_INST, DL_UART_INTERRUPT_RX);
    DL_UART_clearInterruptStatus(UART_1_INST, DL_UART_INTERRUPT_RX);
    DL_UART_clearInterruptStatus(UART_2_INST, DL_UART_INTERRUPT_RX);
    DL_UART_clearInterruptStatus(UART_3_INST, DL_UART_INTERRUPT_RX);
}

/* =========================================================================
   【纯净串口中断服务程序 (ISR)】- 已移除所有业务解析逻辑
   ========================================================================= */

void UART_0_INST_IRQHandler(void)
{
    if(DL_UART_getEnabledInterruptStatus(UART_0_INST, DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
    {
        volatile uint8_t ch = DL_UART_receiveData(UART_0_INST); // 仅读取清除硬件缓存
        DL_UART_clearInterruptStatus(UART_0_INST, DL_UART_INTERRUPT_RX);
    }
}

void UART_1_INST_IRQHandler(void)
{
    if(DL_UART_getEnabledInterruptStatus(UART_1_INST, DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
    {
        volatile uint8_t ch = DL_UART_receiveData(UART_1_INST);
        DL_UART_clearInterruptStatus(UART_1_INST, DL_UART_INTERRUPT_RX);
    }
}

void UART_2_INST_IRQHandler(void)
{
    if(DL_UART_getEnabledInterruptStatus(UART_2_INST, DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
    {
        volatile uint8_t ch = DL_UART_receiveData(UART_2_INST);
        DL_UART_clearInterruptStatus(UART_2_INST, DL_UART_INTERRUPT_RX);
    }
}

void UART_3_INST_IRQHandler(void)
{
    if(DL_UART_getEnabledInterruptStatus(UART_3_INST, DL_UART_INTERRUPT_RX) == DL_UART_INTERRUPT_RX)
    {
        volatile uint8_t ch = DL_UART_receiveData(UART_3_INST);
        DL_UART_clearInterruptStatus(UART_3_INST, DL_UART_INTERRUPT_RX);
    }
}

/* =========================================================================
   【无名创新通用标准输出驱动区】
   ========================================================================= */

/**
 * @brief 发送指定串口的字符串（以 \0 结尾）
 * @param port 串口外设指针 (如 UART_0_INST, UART_2_INST 等)
 * @param str  字符串首地址
 */
void UART_SendString(UART_Regs *port, char *str)
{
    while(str && *str)
    {
        DL_UART_Main_transmitDataBlocking(port, (uint8_t)(*str));
        str++;
    }
}

/**
 * @brief 发送 N 个字节长度的数据（阻塞安全式）
 */
void UART_SendBytes(UART_Regs *port, uint8_t *ubuf, uint32_t len)
{
    while(len--)
    {
        DL_UART_Main_transmitDataBlocking(port, *ubuf);
        ubuf++;
    }
}

/**
 * @brief 发送 1 个字节的数据
 */
void UART_SendByte(UART_Regs *port, uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(port, data);
}


