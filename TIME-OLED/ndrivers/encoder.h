#ifndef _HW_ENCODER_H_
#define _HW_ENCODER_H_

#include "ti_msp_dl_config.h"

// 旋转方向枚举
typedef enum {
    FORWARD,  // 正向
    REVERSAL  // 反向
} ENCODER_DIR;

// 编码器数据结构体
typedef struct {
    volatile int32_t temp_count; // 保存实时计数值（外部中断中修改，用32位足够）
    int32_t count;               // 根据定时器周期更新的计数值（用于PID控制）
    ENCODER_DIR dir;             // 旋转方向
} ENCODER_RES;

// 函数声明
void encoder_init(void);
void encoder_update(void);

int32_t get_left_encoder_count(void);
ENCODER_DIR get_left_encoder_dir(void);

int32_t get_right_encoder_count(void);
ENCODER_DIR get_right_encoder_dir(void);

#endif /* _HW_ENCODER_H_ */
