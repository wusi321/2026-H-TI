#include "encoder.h"


// 定义左、右两个电机的编码器数据结构
static ENCODER_RES left_encoder;
static ENCODER_RES right_encoder;

// 编码器初始化
void encoder_init(void)
{
    // 所有编码器引脚现在都在 PORTA，只需使能 GPIOA 中断
    NVIC_ClearPendingIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    
    // 结构体数据清零
    left_encoder.temp_count = 0;
    left_encoder.count = 0;
    left_encoder.dir = FORWARD;

    right_encoder.temp_count = 0;
    right_encoder.count = 0;
    right_encoder.dir = FORWARD;
}

// 获取左编码器的值
int32_t get_left_encoder_count(void)
{
    return left_encoder.count;
}

// 获取左编码器的方向
ENCODER_DIR get_left_encoder_dir(void)
{
    return left_encoder.dir;
}

// 获取右编码器的值
int32_t get_right_encoder_count(void)
{
    return right_encoder.count;
}

// 获取右编码器的方向
ENCODER_DIR get_right_encoder_dir(void)
{
    return right_encoder.dir;
}

// 编码器数据更新：请在 10ms 的定时器中断（TIMER_TICK）中调用此函数
void encoder_update(void)
{
    // 1. 更新左电机数据
    left_encoder.count = left_encoder.temp_count;
    left_encoder.dir = (left_encoder.count >= 0) ? FORWARD : REVERSAL;
    left_encoder.temp_count = 0; // 计数值清零

    // 2. 更新右电机数据
    right_encoder.count = right_encoder.temp_count;
    right_encoder.dir = (right_encoder.count >= 0) ? FORWARD : REVERSAL;
    right_encoder.temp_count = 0; // 计数值清零
}

// MSPM0G 的 GPIOA 中断由 GROUP1 中断服务函数统一管理
void GROUP1_IRQHandler(void)
{
    // 获取当前触发的是哪个 GPIO 端口组的中断
    switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
        
        // ---------------- PORTA 中断处理 ----------------
        // 此时编码器全搬到了 PORTA (PA12, PA13, PA14, PA15)
        case DL_INTERRUPT_GROUP1_IIDX_GPIOA: {
            // 1. 同时获取这四个引脚的中断触发状态
            uint32_t gpio_status_a = DL_GPIO_getEnabledInterruptStatus(PORTA_PORT, 
                                     PORTA_E1A_PIN | PORTA_E1B_PIN | PORTA_E2A_PIN | PORTA_E2B_PIN);
            
            // 2. 一次性读取整个 PORTA 的电平快照，避免多次调用 readPins 引起的高频时序错位
            uint32_t porta_pins = DL_GPIO_readPins(PORTA_PORT, 
                                  PORTA_E1A_PIN | PORTA_E1B_PIN | PORTA_E2A_PIN | PORTA_E2B_PIN);

            /* ================= 左电机 (E1A / E1B) 处理 ================= */
            
            // E1A (PA13) 上升沿触发
            if ((gpio_status_a & PORTA_E1A_PIN) == PORTA_E1A_PIN) {
                // 判断此时 E1B (PA14) 是否为低电平
                if ((porta_pins & PORTA_E1B_PIN) == 0) {
                    left_encoder.temp_count--;
                } else {
                    left_encoder.temp_count++;
                }
            }
            
            // E1B (PA14) 上升沿触发 （独立 if，防止两个边缘信号极其接近时产生丢失）
            if ((gpio_status_a & PORTA_E1B_PIN) == PORTA_E1B_PIN) {
                // 判断此时 E1A (PA13) 是否为低电平
                if ((porta_pins & PORTA_E1A_PIN) == 0) {
                    left_encoder.temp_count++;
                } else {
                    left_encoder.temp_count--;
                }
            }

            /* ================= 右电机 (E2A / E2B) 处理 ================= */
            
            // E2A (PA15) 上升沿触发
            if ((gpio_status_a & PORTA_E2A_PIN) == PORTA_E2A_PIN) {
                // 判断此时 E2B (PA12) 是否为低电平
                if ((porta_pins & PORTA_E2B_PIN) == 0) {
                    right_encoder.temp_count++;
                } else {
                    right_encoder.temp_count--;
                }
            }
            
            // E2B (PA12) 上升沿触发
            if ((gpio_status_a & PORTA_E2B_PIN) == PORTA_E2B_PIN) {
                // 判断此时 E2A (PA15) 是否为低电平
                if ((porta_pins & PORTA_E2A_PIN) == 0) {
                    right_encoder.temp_count--;
                } else {
                    right_encoder.temp_count++;
                }
            }
            
            // 3. 彻底清除 PORTA 这一组的所有相关中断标志位
            DL_GPIO_clearInterruptStatus(PORTA_PORT, PORTA_E1A_PIN | PORTA_E1B_PIN | PORTA_E2A_PIN | PORTA_E2B_PIN);
            break;
        }
        
        default:
            break;
    }
}