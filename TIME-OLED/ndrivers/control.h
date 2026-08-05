#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "ti_msp_dl_config.h"

#define LOCK   1
#define UNLOCK 0

extern float speed_setup;       // 速度设定基础值

/* --- 外部监控交互 API --- */
int32_t get_track_error(void);         
int32_t get_steering_loop_output(void); 
int32_t get_left_motor_pwm(void);       
int32_t get_right_motor_pwm(void);      
int32_t get_car_lock_status(void);
uint32_t get_task2_run_time(void);
/* --- 循迹状态与核心控制 API --- */
typedef struct {
    uint8_t gray_bit[5];        
    uint16_t state;             
} GRAY_STATE;

void track_control_init(void);
void track_control_loop(void);
void motor_fixed_speed_loop(void);

#endif /* __CONTROL_H__ */
