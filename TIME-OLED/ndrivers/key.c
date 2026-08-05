#include "key.h"
#include "systick.h"  // 引入全新的延时库用于消抖
#include "control.h"
#include "uart.h"     // 确保引入了串口相关的头文件

/* --- 引用外部控制参数 --- */
extern float PID_STEER_KP;    
extern float PID_STEER_KD;    

/* --- 引用外部状态标志位与任务变量 --- */
extern uint8_t car_stop_state; 
extern uint8_t car_start_flag; 
extern uint8_t current_task;   // 获取当前任务标号

/**
 * @brief 智能双向按键扫描与消抖状态机
 */
void scan_tuning_keys(void)
{
    // 1. KEY (PORTB_KEY_PIN)：一键启动发车并下发 K230 指令
    if (DL_GPIO_readPins(PORTB_PORT, PORTB_KEY_PIN) == 0)
    {
        delay_ms(10); // 消抖
        if (DL_GPIO_readPins(PORTB_PORT, PORTB_KEY_PIN) == 0)
        {
            // 允许在初始未发车(0) 或 任务结束停车(4) 的状态下重新发车
            if (car_stop_state == 0 || car_stop_state == 4) 
            {
                car_stop_state = 0; // 强制重置状态机，让 track_control_loop 能捕捉到发车
                car_start_flag = 1; 
            }
            
            /* =======================================================
               功能：组装并发送 K230 赛题任务指令
               帧头: 0xFF | 数据: current_task | 帧尾: 0x0D
               ======================================================= */
            uint8_t k230_cmd[3];
            k230_cmd[0] = 0xFF;
            k230_cmd[1] = current_task;  // 发送当前的赛题任务号 (1~6)
            k230_cmd[2] = 0x0D;

            for(uint8_t i = 0; i < 3; i++)
            {
                DL_UART_Main_transmitDataBlocking(UART_3_INST, k230_cmd[i]);
            }
            /* ======================================================= */

            // 松手检测死循环
            while (DL_GPIO_readPins(PORTB_PORT, PORTB_KEY_PIN) == 0); 
        }
    }
    
    // 2. KEY_BSL (PORTA_KEY_BSL_PIN)：赛题任务标号切换 (1~6循环)
    if ((DL_GPIO_readPins(PORTA_PORT, PORTA_KEY_BSL_PIN) & PORTA_KEY_BSL_PIN) != 0)
    {
        delay_ms(10); // 消抖
        if ((DL_GPIO_readPins(PORTA_PORT, PORTA_KEY_BSL_PIN) & PORTA_KEY_BSL_PIN) != 0)
        {
            // 允许在初始未发车(0) 或 任务结束停车(4) 的状态下切换任务
            if (car_stop_state == 0 || car_stop_state == 4)
            {
                current_task++;
                if (current_task > 6)
                {
                    current_task = 1; // 超过 6 归零回 1
                }
            }
            
            // 松手检测死循环
            while ((DL_GPIO_readPins(PORTA_PORT, PORTA_KEY_BSL_PIN) & PORTA_KEY_BSL_PIN) != 0); 
        }
    }
}