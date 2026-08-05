#include "systick.h"

// 默认内核时钟为 32MHz (32LL)，根据实际 SysConfig 配置，如果是 80MHz 请改为 80LL
#define CPU_CLK_MHZ     32LL 

/**
 * @brief  初始化滴答定时器
 */
void systick_init(void)
{
    // 1. 设置重装载寄存器为最大值 0xFFFFFF (24位计数器)
    SysTick->LOAD  = 0xFFFFFFUL; 
    
    // 2. 清空当前计数值
    SysTick->VAL   = 0UL;        
    
    // 3. 配置控制寄存器：选择内核时钟源 (CLKSOURCE=1)，并使能计数器 (ENABLE=1)
    //    注意：这里不要开启中断 (TICKINT=0)，让它作为一个纯粹的硬件定时器运行
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; 
}

/**
 * @brief  微秒级精确延时
 * @param  us: 延时的微秒数
 */
void delay_us(unsigned int us)
{
    uint32_t ticks = us * CPU_CLK_MHZ;
    uint32_t start_tick = SysTick->VAL;
    uint32_t current_tick;

    while (1)
    {
        current_tick = SysTick->VAL;
        // 因为 SysTick 是向下递减计数
        if (current_tick < start_tick)
        {
            if ((start_tick - current_tick) >= ticks) break;
        }
        else
        {
            if (((SysTick->LOAD - current_tick) + start_tick) >= ticks) break;
        }
    }
}

/**
 * @brief  毫秒级延时
 * @param  ms: 延时的毫秒数
 */
void delay_ms(unsigned int ms)
{
    while (ms--)
    {
        delay_us(1000);
    }
}