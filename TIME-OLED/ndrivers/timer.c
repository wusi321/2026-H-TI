#include "timer.h"
#include "encoder.h"
#include "control.h"
#include "bsp_mpu6050.h"
#include "inv_mpu.h"
#include "uart.h"
#include <stdio.h>


//float pitch = 0, roll = 0, yaw = 0; //欧拉角

/**
 * @brief 定时器中断通道使能
 */
void timer_init(void)
{
    // 清除并启动中断 NVIC 通道
    NVIC_ClearPendingIRQ(TIMER_TICK_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_TICK_INST_INT_IRQN);

    // 2. ?? 新增：启动 100ms 数据读取定时器中断 (TIMA)
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);

}
static uint32_t led_blink_cnt = 0; // 静态变量，保持状态
/**
 * @brief TIMA1 (TIMER_TICK) 10ms 定时器中断服务函数
 */
void TIMER_TICK_INST_IRQHandler(void)
{
    // 检查是否为计数器归零 (ZERO) 周期事件
    if (DL_TimerA_getPendingInterrupt(TIMER_TICK_INST) == DL_TIMERA_IIDX_ZERO)
    {
        // 1. 刷新捕获并锁存这 10ms 内的编码器脉冲数
        encoder_update();
        // 2. 核心控制环路运行
        // 注意：该函数内绝对不能包含任何 delay、printf 或耗时的 OLED 刷新函数！
        track_control_loop();      // 如果要走线，保持这行使能
        //motor_fixed_speed_loop(); // 如果要固定速度测平衡，用这一行
        
    }
}

/**
 * @brief ?? 新增：TIMA0 (TIMER_0) 100ms 定时器中断服务函数
 * @note  负责低频耗时任务：读取陀螺仪、串口打印轮速与姿态
 */
void TIMER_0_INST_IRQHandler(void)
{
    // 检查是否为高级定时器 A0 的归零事件
    if (DL_TimerA_getPendingInterrupt(TIMER_0_INST) == DL_TIMERA_IIDX_ZERO)
    {
        // 1. 自动重启计数（因为 SysConfig 里默认配置成了 ONE_SHOT 单次模式）
        DL_TimerA_startCounter(TIMER_0_INST);

//        if( mpu_dmp_get_data(&pitch, &roll, &yaw) == 0 )
//        {
//            printf("%.1f, %.1f, %.1f\r\n", pitch, roll, yaw);
//        }
    }
}
