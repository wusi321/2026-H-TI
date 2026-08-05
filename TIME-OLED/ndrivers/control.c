#include "control.h"
#include "encoder.h"
#include "uart.h"
#include <stdio.h>

/* =========================================================================
   【调参专区】PID 全套参数与结构
   ========================================================================= */
uint8_t speed_pid_mode  = 1;        // 0:位置式PID控制、1:增量式PID控制（默认开启）
uint8_t speed_ctrl_mode = 1;        // 速度控制模式：1-两轮独立控制

// 1. 50Hz 转向控制参数（位置式 PD，20ms周期）
float PID_STEER_KP   = 10.0f;
float PID_STEER_KD   = 30.0f;
float steer_deadzone = 0.0f;       // 转向死区补偿
float turn_output    = 0.0f;
float turn_output_last = 0.0f;
float turn_ctrl_pwm  = 0.0f;        // 转向环输出量
float turn_scale     = 0.15f;       // 转向系数：决定差速强度

// 2. 100Hz 速度控制器参数
float speed_setup    = 12.0f;       // 速度设定基础值
float speed_kp       = 90.0f;
float speed_ki       = 5.0f;

#define speed_err_max         50.0f  // 速度偏差限幅值
#define speed_integral_max    600.0f // 位置式积分限幅值
#define speed_ctrl_output_max 950    // 控制器输出总限幅值

/* --- 全局状态变量 --- */
static GRAY_STATE gray_state;
static uint8_t unlock_flag = UNLOCK;

static float gray_status[1] = {0.0f};
static float gray_status_backup[1][20] = {0.0f};
static uint32_t gray_status_worse = 0;

float speed_expect[2]       = {0.0f, 0.0f}; 
float speed_feedback[2]     = {0.0f, 0.0f};   
float speed_error[2]        = {0.0f, 0.0f};   
float speed_integral[2]     = {0.0f, 0.0f};   
float speed_output[2]       = {0.0f, 0.0f};   

float speed_output_delta[2] = {0.0f, 0.0f};
float speed_output_last[2]  = {0.0f, 0.0f};
float speed_error_last[2]   = {0.0f, 0.0f};

float steer_smooth_output = 0.0f;             

/* --- 停车与发车状态机控制变量 --- */
uint8_t  car_stop_state = 0;        
uint16_t car_stop_timer = 0;        
uint8_t  car_start_flag = 0;        

#define  DEPART_BLIND_TIME 1000     
#define  BRAKE_TIME        150      
#define  BRAKE_SPEED       -25.0f   

/* --- 赛题任务控制变量 --- */
extern uint8_t current_task;     
float ball_target_pos = 0.0f;    // 钢球期望位置 (单位: cm)
float arbitrary_pos = 3.0f;      // 任意指定位置 (用于任务6)
static uint32_t task3_timer = 0; 
uint32_t task2_run_time_ms = 0;

/* =========================================================================
   【任务2、4、5、6专区】梯形加减速脉冲控制参数
   ========================================================================= */
float car_run_distance = 0.0f;       // 统一的累计脉冲距离（实时）
float dynamic_run_speed = 0.0f;      // 统一的动态梯形期望速度

// ---------------- 任务4专属参数 (定脉冲绝对停车) ----------------
#define TASK4_TOTAL_PULSE   7000.0f   // 总移动脉冲数 (达到该值后完全停车)
#define TASK4_ACCEL_PULSE   1500.0f   // 加速段脉冲数 
#define TASK4_DECEL_PULSE   1500.0f   // 减速段脉冲数 
#define TASK4_MAX_SPEED     12.0f     // 匀速段最大速度 
#define TASK4_MIN_SPEED     2.0f      // 起步与停车前的最小速度 

// ---------------- 任务5、6专属参数 (独立定脉冲绝对停车) ----------------
#define TASK56_TOTAL_PULSE  22000.0f  // 总移动脉冲数 (达到该值后完全停车)
#define TASK56_ACCEL_PULSE  1500.0f   // 加速段脉冲数 
#define TASK56_DECEL_PULSE  2000.0f   // 减速段脉冲数 
#define TASK56_MAX_SPEED    12.0f     // 匀速段最大速度 
#define TASK56_MIN_SPEED    2.0f      // 起步与停车前的最小速度 

// ---------------- 任务2专属参数 (脉冲减速 + 寻线特殊标志停车) ----------------
#define TASK2_ACCEL_PULSE   1500.0f   // [阶段1] 加速段脉冲数 
#define TASK2_TARGET_PULSE  18000.0f  // [阶段2] 维持高速的脉冲阈值 (达到该里程后开始减速找线)
#define TASK2_DECEL_PULSE   2000.0f   // [阶段3] 减速段所需脉冲数 (匀减速持续的距离)
#define TASK2_MAX_SPEED     15.0f     // 匀速段最大速度
#define TASK2_MIN_SPEED     6.0f      // 起步速度与减速后的[阶段4]寻找线蠕行速度

/* --- 内部私有辅助函数声明 --- */
static void gpio_input_check_channel_5(void);
static void set_motor_pwm(int32_t left_pwm, int32_t right_pwm);

static inline float constrain_float(float amt, float low, float high)
{
    if (amt < low)  return low;
    if (amt > high) return high;
    return amt;
}

void track_control_init(void)
{
    turn_output = 0.0f;
    turn_output_last = 0.0f;
    turn_ctrl_pwm = 0.0f;
    steer_smooth_output = 0.0f;
    gray_status[0] = 0.0f;
    gray_status_worse = 0;
    unlock_flag = UNLOCK;
    
    car_stop_state = 0;
    car_stop_timer = 0;
    car_start_flag = 0; 

    for(int i = 0; i < 2; i++)
    {
        speed_error[i]       = 0.0f;
        speed_error_last[i]  = 0.0f;
        speed_integral[i]    = 0.0f;
        speed_output[i]      = 0.0f;
        speed_output_last[i] = 0.0f;
        speed_output_delta[i] = 0.0f;
    }
}

static void gpio_input_check_channel_5(void)
{
    uint32_t porta_pins = DL_GPIO_readPins(PORTA_PORT,
                                           PORTA_L2_PIN | PORTA_L1_PIN | PORTA_M_PIN | PORTA_R1_PIN | PORTA_R2_PIN);

    gray_state.gray_bit[0] = (porta_pins & PORTA_L2_PIN) ? 1 : 0;
    gray_state.gray_bit[1] = (porta_pins & PORTA_L1_PIN) ? 1 : 0;
    gray_state.gray_bit[2] = (porta_pins & PORTA_M_PIN)  ? 1 : 0;
    gray_state.gray_bit[3] = (porta_pins & PORTA_R1_PIN) ? 1 : 0;
    gray_state.gray_bit[4] = (porta_pins & PORTA_R2_PIN) ? 1 : 0;

    gray_state.state = 0x0000;
    for(uint16_t i = 0; i < 5; i++)
    {
        gray_state.state |= (gray_state.gray_bit[i] << i);
    }

    for(uint16_t i = 19; i > 0; i--)
    {
        gray_status_backup[0][i] = gray_status_backup[0][i - 1];
    }
    gray_status_backup[0][0] = gray_status[0];

    switch(gray_state.state)
    {
    case 0x0001: gray_status[0] =  4.0f; gray_status_worse /= 2; break;
    case 0x0003: gray_status[0] =  3.0f; gray_status_worse /= 2; break;
    case 0x0002: gray_status[0] =  2.0f; gray_status_worse /= 2; break;
    case 0x0006: gray_status[0] =  1.0f; gray_status_worse /= 2; break;
    case 0x0004: gray_status[0] =  0.0f; gray_status_worse /= 2; break;
    case 0x000C: gray_status[0] = -1.0f; gray_status_worse /= 2; break;
    case 0x0008: gray_status[0] = -2.0f; gray_status_worse /= 2; break;
    case 0x0018: gray_status[0] = -3.0f; gray_status_worse /= 2; break;
    case 0x0010: gray_status[0] = -4.0f; gray_status_worse /= 2; break;
    case 0x0000:
    default:
        gray_status[0] = gray_status_backup[0][0];
        gray_status_worse++;
        break;
    }
}

void gray_turn_control(float *output)
{
    turn_output_last = turn_output;

    float current_steer_error = 0.0f - gray_status[0];
    static float last_steer_error = 0.0f;
    float steer_deriv = current_steer_error - last_steer_error;
    last_steer_error = current_steer_error;

    turn_output = (PID_STEER_KP * current_steer_error) + (PID_STEER_KD * steer_deriv);

    if(turn_output > 0.0f) turn_output += steer_deadzone;
    if(turn_output < 0.0f) turn_output -= steer_deadzone;

    if (turn_output > 500.0f)  turn_output = 500.0f;
    if (turn_output < -500.0f) turn_output = -500.0f;

    steer_smooth_output = 0.75f * turn_output + 0.25f * turn_output_last;

    turn_ctrl_pwm = steer_smooth_output;
    *output = steer_smooth_output;
}

void speed_control(void)
{
    if(unlock_flag == LOCK || speed_ctrl_mode != 1)
    {
        speed_output[0] = 0.0f; speed_output_last[0] = 0.0f; speed_integral[0] = 0.0f;
        speed_output[1] = 0.0f; speed_output_last[1] = 0.0f; speed_integral[1] = 0.0f;
        set_motor_pwm(0, 0);
        return;
    }

    float target_left  = speed_expect[0];
    float target_right = speed_expect[1];

    if (speed_pid_mode == 0) // 位置式
    {
        // 左电机
        speed_feedback[0] = (float)get_left_encoder_count();
        speed_error[0]    = target_left - speed_feedback[0];
        speed_error[0]    = constrain_float(speed_error[0], -speed_err_max, speed_err_max);
        speed_integral[0] += speed_ki * speed_error[0];
        speed_integral[0] = constrain_float(speed_integral[0], -speed_integral_max, speed_integral_max);
        speed_output[0]   = speed_integral[0] + speed_kp * speed_error[0];
        speed_output[0]   = constrain_float(speed_output[0], -(float)speed_ctrl_output_max, (float)speed_ctrl_output_max);

        // 右电机
        speed_feedback[1] = (float)get_right_encoder_count();
        speed_error[1]    = target_right - speed_feedback[1];
        speed_error[1]    = constrain_float(speed_error[1], -speed_err_max, speed_err_max);
        speed_integral[1] += speed_ki * speed_error[1];
        speed_integral[1] = constrain_float(speed_integral[1], -speed_integral_max, speed_integral_max);
        speed_output[1]   = speed_integral[1] + speed_kp * speed_error[1];
        speed_output[1]   = constrain_float(speed_output[1], -(float)speed_ctrl_output_max, (float)speed_ctrl_output_max);
    }
    else // 增量式
    {
        // 左电机
        speed_feedback[0] = (float)get_left_encoder_count();
        float current_err_left = target_left - speed_feedback[0];
        current_err_left = constrain_float(current_err_left, -speed_err_max, speed_err_max);
        speed_output_delta[0] = speed_kp * (current_err_left - speed_error_last[0]) + speed_ki * current_err_left;
        speed_output[0] = speed_output_last[0] + speed_output_delta[0];
        speed_output[0] = constrain_float(speed_output[0], -(float)speed_ctrl_output_max, (float)speed_ctrl_output_max);
        speed_error_last[0]  = current_err_left;
        speed_output_last[0] = speed_output[0];

        // 右电机
        speed_feedback[1] = (float)get_right_encoder_count();
        float current_err_right = target_right - speed_feedback[1];
        current_err_right = constrain_float(current_err_right, -speed_err_max, speed_err_max);
        speed_output_delta[1] = speed_kp * (current_err_right - speed_error_last[1]) + speed_ki * current_err_right;
        speed_output[1] = speed_output_last[1] + speed_output_delta[1];
        speed_output[1] = constrain_float(speed_output[1], -(float)speed_ctrl_output_max, (float)speed_ctrl_output_max);
        speed_error_last[1]  = current_err_right;
        speed_output_last[1] = speed_output[1];
    }
    
    set_motor_pwm((int32_t)speed_output[0], (int32_t)speed_output[1]);
}

/**
 * @brief 赛题任务调度管理机
 */
void task_manager(void)
{
    if (car_stop_state == 0) 
    {
        task3_timer = 0;
        return; 
    }

    switch(current_task)
    {
        case 1: ball_target_pos = 0.0f; break;
        case 2: ball_target_pos = 0.0f; break;
        case 3:
            task3_timer += 10; 
            if (task3_timer < 1500) {
                ball_target_pos = 5.0f;  
            } else {
                ball_target_pos = -5.0f; 
            }
            break;
        case 4: ball_target_pos = 0.0f; break;
        case 5: ball_target_pos = 0.0f; break;
        case 6: ball_target_pos = arbitrary_pos; break;
        default: ball_target_pos = 0.0f; break;
    }
}
/**
 * @brief 寻迹模式总入口（运行于 10ms 中断服务程序）
 */
void track_control_loop(void)
{
    static uint8_t steer_div_cnt = 0;

        // 任务2运行时间累计 (依赖定时器，每10ms周期精准进入一次)
    if (current_task == 2 && (car_stop_state == 1 || car_stop_state == 2))
    {
        task2_run_time_ms += 10;
    }
    
    // 1. 每 10ms 累加编码器脉冲，计算出小车行驶的总距离
    float current_avg_speed = (speed_feedback[0] + speed_feedback[1]) / 2.0f;
    if (current_avg_speed > 0) // 只积分正向位移
    {
        car_run_distance += current_avg_speed;
    }

    steer_div_cnt++;
    if(steer_div_cnt >= 2)
    { 
        steer_div_cnt = 0;
        gpio_input_check_channel_5();
        
        /* ==================== 发车与永久停车 状态机 ==================== */
        uint8_t is_stop_line = 0;
        
        // 仅在任务2到达目标步数(减速段/寻线段)时，开启停车线检测
        if (current_task == 2 && car_run_distance >= TASK2_TARGET_PULSE) 
        {
            // 灰度位定义: bit1=L1, bit2=M, bit3=R1 (熄灭/识别到黑线则为1)
            uint8_t mid_sensor_count = gray_state.gray_bit[1] + gray_state.gray_bit[2] + gray_state.gray_bit[3];
            
            // 标志：中间三个光电管，任意两个或以上熄灭(识别到黑线)就判定为停车线
            if (mid_sensor_count >= 2) 
            {
                is_stop_line = 1;
            }
        }
        // 注：任务4、5、6纯靠脉冲停车，完全无需看停车线，is_stop_line 保持 0。

        if (car_stop_state == 0)
        {
            if (car_start_flag == 1)
            {
                car_stop_state = 1; 
                car_stop_timer = 0;
                car_start_flag = 0; 
                
                // 发车瞬间，重置里程和动态起步速度
                car_run_distance = 0.0f; 
                task2_run_time_ms = 0;// 发车瞬间清空任务2计时
                if(current_task == 4) dynamic_run_speed = TASK4_MIN_SPEED;
                else if(current_task == 5 || current_task == 6) dynamic_run_speed = TASK56_MIN_SPEED;
                else if(current_task == 2) dynamic_run_speed = TASK2_MIN_SPEED;
            }
        }
        else if (car_stop_state == 1)
        {
            car_stop_timer += 20; 
            if (car_stop_timer >= DEPART_BLIND_TIME) car_stop_state = 2; 
        }
        else if (car_stop_state == 2)
        {
            // 【多任务停车触发汇总】
            uint8_t trigger_stop = 0;
            
            // 任务2: 识别到停车标志
            if (is_stop_line) trigger_stop = 1;
            // 任务4: 固定脉冲达标
            if (current_task == 4 && car_run_distance >= TASK4_TOTAL_PULSE) trigger_stop = 1;
            // 任务5/6: 独立固定脉冲达标
            if ((current_task == 5 || current_task == 6) && car_run_distance >= TASK56_TOTAL_PULSE) trigger_stop = 1;

            // 触发停车：直接进入死区状态4并清空PID积分
            if (trigger_stop)
            {
                car_stop_state = 4; 
                for(int i = 0; i < 2; i++)
                {
                    speed_error[i]       = 0.0f;
                    speed_error_last[i]  = 0.0f;
                    speed_integral[i]    = 0.0f;
                    speed_output[i]      = 0.0f;
                    speed_output_last[i] = 0.0f;
                    speed_output_delta[i] = 0.0f;
                }
            }
        }
        
        if (car_stop_state == 0 || car_stop_state == 1 || car_stop_state == 4)
        {
            gray_status[0] = 0.0f; 
        }

        gray_turn_control(&turn_ctrl_pwm);
    }

    // 调用任务调度管理机更新摆球目标等变量
    task_manager();

    /* =========================================================
       【梯形速度生成器 (空间域线性插值)】
       ========================================================= */
    if (car_stop_state == 1 || car_stop_state == 2)
    {
        // --- 任务4的梯形规划 (定总长) ---
        if (current_task == 4)
        {
            if (car_run_distance <= TASK4_ACCEL_PULSE) {
                float progress = car_run_distance / TASK4_ACCEL_PULSE;
                dynamic_run_speed = TASK4_MIN_SPEED + (TASK4_MAX_SPEED - TASK4_MIN_SPEED) * progress;
            } else if (car_run_distance <= (TASK4_TOTAL_PULSE - TASK4_DECEL_PULSE)) {
                dynamic_run_speed = TASK4_MAX_SPEED;
            } else if (car_run_distance < TASK4_TOTAL_PULSE) {
                float remain_dist = TASK4_TOTAL_PULSE - car_run_distance;
                float progress = remain_dist / TASK4_DECEL_PULSE;
                dynamic_run_speed = TASK4_MIN_SPEED + (TASK4_MAX_SPEED - TASK4_MIN_SPEED) * progress;
            } else {
                dynamic_run_speed = 0.0f;
            }
        }
        // --- 任务5、6的梯形规划 (独立定总长) ---
        else if (current_task == 5 || current_task == 6)
        {
            if (car_run_distance <= TASK56_ACCEL_PULSE) {
                float progress = car_run_distance / TASK56_ACCEL_PULSE;
                dynamic_run_speed = TASK56_MIN_SPEED + (TASK56_MAX_SPEED - TASK56_MIN_SPEED) * progress;
            } else if (car_run_distance <= (TASK56_TOTAL_PULSE - TASK56_DECEL_PULSE)) {
                dynamic_run_speed = TASK56_MAX_SPEED;
            } else if (car_run_distance < TASK56_TOTAL_PULSE) {
                float remain_dist = TASK56_TOTAL_PULSE - car_run_distance;
                float progress = remain_dist / TASK56_DECEL_PULSE;
                dynamic_run_speed = TASK56_MIN_SPEED + (TASK56_MAX_SPEED - TASK56_MIN_SPEED) * progress;
            } else {
                dynamic_run_speed = 0.0f;
            }
        }
        // --- 任务2的梯形规划 (减速后找线) ---
        else if (current_task == 2)
        {
            if (car_run_distance <= TASK2_ACCEL_PULSE) {
                float progress = car_run_distance / TASK2_ACCEL_PULSE;
                dynamic_run_speed = TASK2_MIN_SPEED + (TASK2_MAX_SPEED - TASK2_MIN_SPEED) * progress;
            } 
            else if (car_run_distance < TASK2_TARGET_PULSE) {
                dynamic_run_speed = TASK2_MAX_SPEED;
            } 
            else if (car_run_distance < (TASK2_TARGET_PULSE + TASK2_DECEL_PULSE)) {
                float decel_dist = car_run_distance - TASK2_TARGET_PULSE;
                float progress = decel_dist / TASK2_DECEL_PULSE;
                dynamic_run_speed = TASK2_MAX_SPEED - (TASK2_MAX_SPEED - TASK2_MIN_SPEED) * progress;
            } 
            else {
                dynamic_run_speed = TASK2_MIN_SPEED; // 保持极低速，等待黑线触发
            }
        }
    }

    /* =========================================================
       更新期望速度并下发至 PID 闭环控制器
       ========================================================= */
    if (car_stop_state == 0 || car_stop_state == 4)
    {
        speed_expect[0] = 0.0f;
        speed_expect[1] = 0.0f;
        turn_ctrl_pwm = 0.0f;
    }
    else
    {
        if (current_task == 1 || current_task == 3) 
        {
             speed_expect[0] = 0.0f;
             speed_expect[1] = 0.0f;
             turn_ctrl_pwm = 0.0f; 
        }
        else if (current_task == 2 || current_task == 4 || current_task == 5 || current_task == 6) 
        {
             // 任务2、4、5、6均应用平滑加减速速度 (叠加寻迹差速输出)
             speed_expect[0] = dynamic_run_speed + turn_ctrl_pwm * turn_scale;
             speed_expect[1] = dynamic_run_speed - turn_ctrl_pwm * turn_scale;
        }
        else
        {
             // 容错兜底
             speed_expect[0] = speed_setup + turn_ctrl_pwm * turn_scale;
             speed_expect[1] = speed_setup - turn_ctrl_pwm * turn_scale;
        }
    }

    // 执行速度闭环
    speed_control();
}

void motor_fixed_speed_loop(void)
{
    static uint8_t steer_div_cnt = 0;

    steer_div_cnt++;
    if(steer_div_cnt >= 2)
    {
        steer_div_cnt = 0;
        gpio_input_check_channel_5();
        gray_turn_control(&turn_ctrl_pwm);
    }
    printf("%.1f,%.1f\n", speed_feedback[0], speed_feedback[1]);
    unlock_flag = UNLOCK;

    speed_expect[0] = speed_setup;
    speed_expect[1] = speed_setup;

    speed_control();
}

static void set_motor_pwm(int32_t left_pwm, int32_t right_pwm)
{
    if(left_pwm >= 0)
    {
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, (uint32_t)left_pwm, DL_TIMER_CC_0_INDEX);
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, 0, DL_TIMER_CC_1_INDEX);
    }
    else
    {
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, 0, DL_TIMER_CC_0_INDEX);
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, (uint32_t)(-left_pwm), DL_TIMER_CC_1_INDEX);
    }

    if(right_pwm >= 0)
    {
        DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, (uint32_t)right_pwm, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, 0, DL_TIMER_CC_1_INDEX);
    }
    else
    {
        DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, 0, DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, (uint32_t)(-right_pwm), DL_TIMER_CC_1_INDEX);
    }
}

int32_t get_track_error(void) { return (int32_t)gray_status[0]; }
int32_t get_steering_loop_output(void) { return (int32_t)turn_ctrl_pwm; }
int32_t get_left_motor_pwm(void) { return (int32_t)speed_output[0]; }
int32_t get_right_motor_pwm(void) { return (int32_t)speed_output[1]; }
int32_t get_car_lock_status(void) { return (unlock_flag == LOCK) ? 1 : 0; }
uint32_t get_task2_run_time(void) { return task2_run_time_ms; }