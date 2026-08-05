#include "ti_msp_dl_config.h"
#include "oled.h"             // OLED 驱动库
#include "encoder.h"          // 编码器测速库
#include "timer.h"            // 定时器中断库
#include "control.h"          // 控制闭环库
#include "uart.h"             // 引入串口库
#include "systick.h"          // 全局 SysTick 硬件精确延时库
#include "key.h"              // 打包好的全局独立按键库
#include <stdio.h>            // 用于 snprintf 拼接安全字符串
#include "bsp_mpu6050.h"
#include "inv_mpu.h"

/* --- 全局任务与控制变量声明 --- */
uint8_t current_task = 1;     // 全局任务标号，默认任务 1 (范围 1~6)
extern float PID_STEER_KP;
extern float PID_STEER_KD;


/**
 * @brief 强抗干扰 OLED 静态内容及参数框架刷新函数
 */
void oled_draw_static_frame(void)
{
    display_6_8_string(0, 0, "T_s:");// 显示时间标签       
    display_6_8_string(0, 1, "L_Spd: ");
    display_6_8_string(0, 2, "R_Spd: ");
    display_6_8_string(0, 3, "L_PID: ");
    display_6_8_string(0, 4, "R_PID: ");
    display_6_8_string(0, 5, "Err  : ");
    display_6_8_string(0, 6, "Turn : ");
    display_6_8_string(0, 7, "Task : ");// 显示当前任务号

}

int main(void)
{
    // 1. 硬件底层主时钟树与配置链配置初始化
    SYSCFG_DL_init();

    // 2. 初始化封装好的内核滴答定时器
    systick_init();
    
    // 3. 核心控制组件与通信中断初始化
    encoder_init();
    timer_init();            // 10ms 核心中断定时器开启
    track_control_init();
    usart_irq_config();
    
    // 4. OLED 屏幕初次上电初始化与首帧绘制
    oled_init();
    oled_draw_static_frame();
    
    /* === 定义 10 分频计数器 === */
    uint8_t loop_divider = 0;      

    while (1)
    {
        /* === 业务 1：按键检测（全速运行，每 10ms 必定扫描一次，响应极快） === */
        scan_tuning_keys();

        /* === 基础底衬延时：将单次循环死等缩短为 10ms === */
        delay_ms(10);

        /* === 业务 2：10 分频逻辑控制（每转 10 圈 = 100ms 执行一次） === */
        loop_divider++;
        if (loop_divider >= 10)
        {
            loop_divider = 0; // 计步器清零，重新开始下一轮 10 分频
            
            // 1. 正常业务数据实时采集
            int32_t speed_left   = get_left_encoder_count();
            int32_t speed_right  = get_right_encoder_count();
            int32_t pid_out_left = get_left_motor_pwm();
            int32_t pid_out_right= get_right_motor_pwm();
            int32_t track_error  = get_track_error();
            int32_t steering_out = get_steering_loop_output();
            int32_t lock_status  = get_car_lock_status();

            // 3. 动态数据看板刷新
            if(lock_status == 1)
            {
                display_6_8_string(90, 0, "[LOCK]");
            }
            else
            {
                display_6_8_string(90, 0, "[UNLK]");
            }
            
            display_6_8_number_f1(54, 1, (float)speed_left);
            display_6_8_number_f1(54, 2, (float)speed_right);
            display_6_8_number_f1(54, 3, (float)pid_out_left);
            display_6_8_number_f1(54, 4, (float)pid_out_right);
            display_6_8_number_pro(54, 5, track_error);
            display_6_8_number_pro(54, 6, steering_out);
            
            // 刷新当前任务号显示
            display_6_8_number_pro(54, 7, (int32_t)current_task); 
            
            // 刷新任务2运行时间显示
            if (current_task == 2)
            {
                // 保留一位小数显示秒数，例如 12.5 秒
                display_6_8_number_f1(34, 0, (float)get_task2_run_time() / 1000.0f);
            }
            else 
            {
                // 其他任务下清空该位置或显示等待
                display_6_8_string(34, 0, "--- "); 
            }
            // 5. 系统健康运行心跳闪烁
            DL_GPIO_togglePins(PORTB_PORT, PORTB_LED_PIN);
        }
    }
}
