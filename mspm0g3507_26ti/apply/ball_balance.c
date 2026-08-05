#include "ti_msp_dl_config.h"

#include "ball_balance.h"
#include "ftServo.h"
#include "system.h"

#include <math.h>
#include <string.h>

#define MAIXCAM_UART_BAUD_RATE             (115200U)
#define MAIXCAM_PACKET_SIZE                (20U)
#define MAIXCAM_MAGIC_0                    (0xAAU)
#define MAIXCAM_MAGIC_1                    (0x55U)
#define MAIXCAM_PROTOCOL_VERSION           (1U)
#define MAIXCAM_FLAG_VALID                 (0x01U)
#define MAIXCAM_FLAG_MEASURED              (0x02U)
#define MAIXCAM_FLAG_TRACKED               (0x04U)
#define MAIXCAM_CRC_DATA_SIZE              (18U)
#define MAIXCAM_OFFSET_VERSION             (2U)
#define MAIXCAM_OFFSET_FLAGS               (3U)
#define MAIXCAM_OFFSET_SEQUENCE            (4U)
#define MAIXCAM_OFFSET_TIMESTAMP_MS        (6U)
#define MAIXCAM_OFFSET_POSITION_MM         (10U)
#define MAIXCAM_OFFSET_VELOCITY_MM_S       (12U)
#define MAIXCAM_OFFSET_CONFIDENCE_MILLI    (14U)
#define MAIXCAM_OFFSET_PROCESSING_US       (16U)
#define MAIXCAM_OFFSET_CRC16               (18U)
#define MAIXCAM_SEQUENCE_RESET_MS          (500U)

#define BALL_CONTROL_PERIOD_S              (0.005f)
#define BALL_POSITION_PHYSICAL_LIMIT_MM    (130)
#define BALL_VELOCITY_PHYSICAL_LIMIT_MM_S  (3000)
#define BALL_ACCELERATION_PHYSICAL_LIMIT_MM_S2 (10000.0f)
#define BALL_TARGET_LIMIT_MM               (125.0f)
#define BALL_POSITION_GAIN_SCALE_MIN        (0.25f)
#define BALL_POSITION_GAIN_SCALE_MAX        (3.0f)
#define BALL_SERVO_MIN_ANGLE_DEG            (-40.0f)
#define BALL_SERVO_MAX_ANGLE_DEG            (40.0f)
#define BALL_ACCELERATION_DT_MIN_MS          (5U)
#define BALL_ACCELERATION_DT_MAX_MS          (120U)
#define BALL_DEG_TO_RAD                      (0.01745329252f)
#define BALL_RAD_TO_DEG                      (57.29577951f)
#define BALL_TARGET_HOLD_MINIMUM_MOVE_ERROR_MM (10.0f)
#define BALL_TARGET_HOLD_BRAKING_FEEDFORWARD_MIN_DEG (-8.0f)
#define BALL_TARGET_HOLD_TURN_YAW_RATE_DPS   (10.0f)
#define BALL_TARGET_HOLD_TURN_SERVO_LIMIT_DEG (20.0f)
#define BALL_TARGET_HOLD_LAUNCH_SERVO_LIMIT_DEG (36.0f)
#define BALL_TARGET_HOLD_LAUNCH_HOLD_MAX_MS  (2500U)
#define BALL_TARGET_HOLD_LAUNCH_BRAKE_LIMIT_DEG (6.0f)
#define BALL_TARGET_HOLD_LAUNCH_BRAKE_WINDOW_MM (30.0f)
#define BALL_TARGET_HOLD_LAUNCH_BRAKE_SPEED_MM_S (15.0f)

#if UART_1_BAUD_RATE != MAIXCAM_UART_BAUD_RATE
#error "UART1 must be 115200 baud for the MaixCAM vision protocol"
#endif

typedef struct {
    uint8_t flags;
    uint16_t sequence;
    uint32_t timestamp_ms;
    int16_t position_mm;
    int16_t velocity_mm_s;
    uint16_t confidence_milli;
    uint16_t processing_us;
} MaixcamPacket;

typedef struct {
    uint8_t bytes[MAIXCAM_PACKET_SIZE];
    uint8_t index;
    uint32_t valid_frames;
    uint32_t crc_errors;
    uint32_t format_errors;
} MaixcamParser;

typedef struct {
    uint8_t present;
    uint8_t flags;
    uint16_t sequence;
    uint32_t timestamp_ms;
    int16_t position_mm;
    int16_t velocity_mm_s;
    uint16_t confidence_milli;
    uint16_t processing_us;
    uint32_t received_at_ms;
} PublishedVisionSample;

BallBalanceConfig g_ball_balance_config = {
    .position_to_velocity_kp_s = 1.8f,      /* 位置环生成目标速度 */
    .max_target_velocity_mm_s = 80.0f,      /* 用户设置的巡航球速上限8cm/s */
    .minimum_target_velocity_mm_s = 25.0f,  /* 低于2cm/s机构只能启停式运动 */
    .minimum_target_velocity_full_error_mm = 5.0f, /* 距到位窗口10mm内连续降低爬行速度 */
    .motion_prediction_time_s = 0.22f,      /* 补偿视觉和舵机响应延迟，提前减速 */
    .velocity_limit_brake_start_ratio = 0.65f, /* 接近速度上限后逐步撤销继续加速 */
    .velocity_limit_overspeed_kp_s = 4.0f, /* 实测超速制动增益 */
    .velocity_limit_max_brake_mm_s2 = 250.0f, /* 超速时最大制动加速度 */
    .braking_acceleration_mm_s2 = 80.0f,    /* 速度轨迹的计划减速度 */
    .distance_brake_gain = 0.0f,            /* 任务档按需启用停车距离制动 */
    .distance_brake_stop_offset_mm = 0.0f, /* 默认按目标中心规划停车 */
    .velocity_reference_accel_limit_mm_s2 = 120.0f, /* 目标速度斜坡加速度 */
    .motion_phase_velocity_hysteresis_mm_s = 3.0f, /* 阶段切换速度回差，避免边界抖动 */
    .velocity_to_acceleration_kp_s = 6.0f, /* 速度环生成目标加速度 */
    .acceleration_feedforward_gain = 0.18f, /* 速度轨迹加速度弱前馈 */
    .acceleration_limit_near_mm_s2 = 50.0f, /* 目标附近限制反向制动幅度 */
    .acceleration_limit_far_mm_s2 = 100.0f, /* 远处正常加速度上限 */
    .acceleration_limit_brake_mm_s2 = 250.0f, /* 对外部扰动和高速球单独加强制动 */
    .acceleration_limit_full_error_mm = 30.0f, /* 加速度限幅调度距离 */
    .acceleration_filter_alpha = 0.18f,     /* 摄像头新帧加速度估计滤波权重 */
    .gravity_mm_s2 = 9800.0f,              /* 重力加速度9.8m/s2 */
    .rolling_acceleration_ratio = 0.7142857f, /* 实心球纯滚动系数5/7 */
    .acceleration_feedback_gain = 0.03f,   /* 实测加速度仅作弱阻尼，避免噪声放大 */
    .servo_degrees_per_acceleration_mm_s2 = 0.06f, /* 正常速度/加速度环到舵机角度 */
    .velocity_integral_gain_deg_per_mm = 0.08f, /* 仅学习坡度和车体加速度补偿 */
    .velocity_integral_unwind_gain_deg_per_mm = 0.50f, /* 方向错误时快速撤销补偿 */
    .velocity_integral_limit_deg = 4.0f,   /* 禁止积分保存静摩擦启动大角度 */
    .velocity_integral_deadband_mm_s = 2.0f, /* 过滤视觉速度零点抖动 */
    .servo_normal_angle_limit_deg = 32.0f, /* 8mm/17mm对应26.96度，留约1度启动余量 */
    .servo_near_target_min_limit_deg = 4.0f, /* 目标附近静态修正角上限 */
    .servo_near_target_full_error_mm = 50.0f, /* 距目标5cm后恢复完整角度 */
    .servo_near_target_full_brake_velocity_mm_s = 80.0f, /* 高速制动恢复完整角度 */
    .task3_positive_servo_limit_deg = 28.0f, /* 球动后的正向控制限幅，降低到位过冲 */
    .task3_positive_target_velocity_mm_s = 80.0f, /* 任务3正向穿越速度8cm/s */
    .task3_negative_minimum_move_error_mm = 10.0f, /* 负向目标正负1cm内禁止防静止 */
    .task3_speed_anti_decay_ratio = 0.80f, /* 范围外低于目标速度80%时补偿 */
    .vehicle_feedforward_gain = 2.65f, /* 车辆纵向加速度补偿比例 */
    .vehicle_command_acceleration_weight = 0.30f, /* 编码器加速度建立后的指令占比 */
    .vehicle_command_acceleration_lead_weight = 0.65f, /* 编码器响应前的指令提前占比 */
    .vehicle_measured_acceleration_takeover_ratio = 0.75f, /* 实测达到指令加速度75%后完成接管 */
    .vehicle_measured_acceleration_filter_alpha = 0.55f, /* 缩短编码器加速度滤波延迟 */
    .vehicle_command_acceleration_filter_alpha = 0.70f, /* 指令变化快速进入前馈 */
    .vehicle_acceleration_deadband_mm_s2 = 30.0f, /* 抑制匀速时编码器量化微分噪声 */
    .vehicle_acceleration_limit_mm_s2 = 800.0f, /* 前馈使用的车辆加速度限幅 */
    .vehicle_feedforward_servo_limit_deg = 35.0f, /* 前馈仅受舵机机械正负40度限位 */
    .vehicle_feedforward_servo_slew_deg_s = 220.0f, /* 起步/刹车补偿快速建立 */
    .vehicle_feedforward_release_slew_deg_s = 60.0f, /* 起步稳定后缓慢撤销前馈，避免立即回拉 */
    .vehicle_feedforward_command_reserve_ratio = 0.35f,
    .vehicle_braking_servo_extra_deg = 0.0f,
    .vehicle_braking_servo_extra_error_mm = 0.0f,
    .vehicle_braking_feedforward_preload_error_mm = 0.0f,
    .vehicle_turn_compensation_gain_deg_per_dps = 0.10f,
    .vehicle_turn_compensation_deadband_dps = 5.0f,
    .vehicle_turn_compensation_limit_deg = 2.5f,
    .vehicle_turn_compensation_filter_alpha = 0.12f,
    .vehicle_turn_compensation_slew_deg_s = 30.0f,
    .vehicle_launch_hold_ratio = 0.10f,     /* 只跨越电机起动死区，不长期锁住旧方向 */
    .vehicle_launch_detect_speed_cmps = 3.0f, /* 指令速度超过3cm/s后识别起步 */
    .vehicle_launch_settle_speed_error_cmps = 2.0f, /* 实测与指令速度接近后才允许释放 */
    .vehicle_launch_settle_acceleration_mm_s2 = 80.0f, /* 加速度低于此值视为趋于匀速 */
    .vehicle_launch_feedforward_slew_deg_s = 0.0f, /* 0表示沿用常规前馈角速度 */
    .vehicle_launch_settle_ms = 100U,       /* 连续稳定100ms后释放起步保持 */
    .vehicle_launch_hold_max_ms = 300U,     /* 指令/编码器互补接管后立即退出保持 */
    .vehicle_launch_preload_enabled = false, /* 默认不在电机动作前预倾 */
    .servo_accel_slew_deg_s = 30.0f,       /* 舵机机构的受限启动角速度 */
    .servo_brake_slew_deg_s = 50.0f,      /* 高速扰动时约80ms建立15度制动角 */
    .servo_level_slew_deg_s = 1500.0f,      /* 约55ms撤销27度启动角，避免继续加速 */
    .beam_length_mm = 250.0f,              /* 左侧合页到右侧滑杆连接点 */
    .servo_gear_radius_mm = 17.0f,         /* 舵机齿轮节圆半径 */
    .breakaway_rack_travel_mm = 8.0f,      /* 实测开始运动所需滑杆位移 */
    .breakaway_acceleration_margin = 1.15f, /* 启动阈值增加15%余量 */
    .settle_position_tolerance_mm = 7.0f,   /* 默认正负0.7cm，具体任务档可覆盖 */
    .settle_velocity_tolerance_mm_s = 10.0f, /* 控制器归零速度窗口 */
    .servo_neutral_angle_deg = 0.0f,        /* 横杆水平时的舵机角度 */
    .servo_hold_bias_deg = 0.0f,             /* 仅移动档的静态水平补偿 */
    .servo_min_angle_deg = -40.0f,          /* 对应滑杆下降约11.87mm */
    .servo_max_angle_deg = 40.0f,           /* 对应滑杆上升约11.87mm */
    .position_filter_alpha = 0.75f,         /* 位置新样本权重 */
    .velocity_filter_alpha = 0.85f,         /* 摄像头已滤波，MCU仅抑制单帧毛刺 */
    .tracked_gain_scale = 0.50f,            /* 仅预测未测量时的控制输出比例 */
    .minimum_move_error_mm = 5.0f,          /* 超出任务允许的0.5cm误差才启用防静止 */
    .minimum_move_stationary_delta_mm = 2.0f, /* 350ms内位移不超过2mm才视为静止 */
    .minimum_move_release_speed_mm_s = 10.0f, /* 朝目标达到2cm/s后交还串级控制 */
    .minimum_move_acceleration_mm_s2 = 20.0f, /* 防静止从约0.8度舵机偏角起步 */
    .minimum_move_acceleration_max_mm_s2 = 100.0f, /* 防静止最大约4度舵机偏角 */
    .minimum_move_acceleration_ramp_mm_s3 = 20.0f, /* 缓慢增加启动加速度 */
    .minimum_move_servo_start_deg = 24.0f, /* 接近实测26.96度静摩擦启动角 */
    .minimum_move_servo_max_deg = 27.0f,   /* 略高于8mm齿条行程对应的26.96度 */
    .minimum_move_servo_ramp_deg_s = 8.0f, /* 缓慢越过静摩擦阈值，球动即撤销 */
    .minimum_move_servo_slew_deg_s = 50.0f, /* 静摩擦修正不额外限制任务3舵机响应 */
    .negative_near_minimum_move_error_max_mm = 0.0f, /* 默认禁用负侧独立修正 */
    .negative_near_minimum_move_release_speed_mm_s = 0.0f,
    .negative_near_minimum_move_servo_start_deg = 0.0f,
    .negative_near_minimum_move_servo_max_deg = 0.0f,
    .negative_near_minimum_move_servo_ramp_deg_s = 0.0f,
    .negative_near_minimum_move_servo_slew_deg_s = 0.0f,
    .positive_near_minimum_move_error_max_mm = 0.0f,
    .positive_near_minimum_move_release_speed_mm_s = 0.0f,
    .negative_return_assist_error_max_mm = 0.0f, /* 默认禁用负侧回中辅助 */
    .negative_return_assist_speed_mm_s = 0.0f,
    .negative_return_assist_release_error_mm = 0.0f,
    .negative_return_assist_brake_limit_mm_s2 = 0.0f,
    .servo_speed_full_error_mm = 50.0f,     /* 动态舵机速度达到高值的距离尺度 */
    .servo_speed_full_velocity_mm_s = 100.0f, /* 球速达到8cm/s时舵机使用最高动态速度 */
    .servo_speed_full_angle_delta_deg = 18.0f, /* 动态舵机速度达到高值的角差尺度 */
    .vision_timeout_ms = 100U,              /* 视觉数据超时后立即回中 */
    .vehicle_braking_vision_timeout_ms = 100U,
    .minimum_move_detect_ms = 150U,         /* 参考已调参工程，确认静止后再启动 */
    .minimum_move_cooldown_ms = 250U,       /* 交还串级后禁止立即反向重复启动 */
    .launch_compensation_requires_stationary = false, /* 静置任务保留原任务3起动特性 */
    .minimum_confidence_milli = 250U,       /* 最低可接受置信度 */
    .servo_speed = 1200U,                   /* 回平时提高舵机跟随速度 */
    .servo_speed_min = 800U,                /* 角度斜坡负责平滑，舵机只需跟随 */
    .servo_speed_max = 1800U,              /* 大误差和制动时提高响应速度 */
    .vehicle_feedforward_direction = 1     /* 正车加速时补偿球向正坐标加速 */
};

BallBalanceStatus g_ball_balance_status;
volatile int8_t g_ball_balance_servo_direction =
    BALL_BALANCE_SERVO_DIRECTION_REVERSED;
volatile float g_ball_balance_target_mm = 0.0f;
volatile bool g_ball_balance_enabled = false;

static MaixcamParser g_maixcam_parser;
static volatile PublishedVisionSample g_published_sample;
static volatile uint32_t g_publish_revision;
static volatile uint32_t g_accepted_packet_count;
static volatile uint32_t g_crc_error_count;
static volatile uint32_t g_format_error_count;
static volatile uint32_t g_sequence_drop_count;
static uint16_t g_last_rx_sequence;
static uint32_t g_last_sequence_received_ms;
static bool g_rx_sequence_initialized;

static uint32_t g_last_control_revision;
static float g_filtered_position_mm;
static float g_filtered_velocity_mm_s;
static float g_filtered_acceleration_mm_s2;
static float g_previous_acceleration_velocity_mm_s;
static uint32_t g_previous_acceleration_timestamp_ms;
static float g_target_velocity_mm_s;
static float g_target_reference_mm;
static float g_last_servo_angle_deg;
static float g_velocity_integral_servo_deg;
static float g_target_hold_bias_deg;
static bool g_target_hold_tune_enabled;
static float g_position_gain_scale = 1.0f;
static float g_motion_direction;
static float g_profile_peak_velocity_mm_s;
static float g_profile_braking_velocity_mm_s;
static float g_minimum_move_anchor_position_mm;
static float g_minimum_move_acceleration_mm_s2;
static float g_minimum_move_servo_angle_deg;
static float g_minimum_move_direction;
static uint32_t g_minimum_move_anchor_received_ms;
static uint32_t g_minimum_move_elapsed_ms;
static uint32_t g_minimum_move_active_started_ms;
static uint32_t g_minimum_move_cooldown_started_ms;
static bool g_minimum_move_observation_active;
static bool g_minimum_move_active;
static bool g_minimum_move_cooldown_active;
static bool g_negative_return_assist_active;
static bool g_motion_profile_initialized;
static BallBalanceMotionPhase g_motion_phase;
static bool g_acceleration_filter_initialized;
static bool g_filter_initialized;
static bool g_was_vision_usable;
static bool g_vehicle_feedforward_enabled;
static bool g_vehicle_motion_initialized;
static float g_vehicle_previous_measured_speed_cmps;
static float g_vehicle_previous_command_speed_cmps;
static float g_vehicle_measured_sample_elapsed_s;
static float g_vehicle_command_sample_elapsed_s;
static float g_vehicle_measured_filter_decay_elapsed_s;
static float g_vehicle_command_filter_decay_elapsed_s;
static float g_vehicle_measured_acceleration_filter_mm_s2;
static float g_vehicle_command_acceleration_filter_mm_s2;
static float g_vehicle_measured_acceleration_mm_s2;
static float g_vehicle_command_acceleration_mm_s2;
static float g_vehicle_feedforward_acceleration_mm_s2;
static float g_vehicle_feedforward_servo_deg;
static float g_vehicle_feedforward_motion_scale = 1.0f;
static float g_vehicle_feedforward_motion_sign;
static float g_vehicle_filtered_yaw_rate_dps;
static float g_vehicle_turn_compensation_servo_deg;
static bool g_vehicle_launch_armed;
static bool g_vehicle_launch_hold_active;
static float g_vehicle_launch_hold_servo_deg;
static float g_vehicle_launch_motion_direction;
static float g_vehicle_launch_elapsed_s;
static float g_vehicle_launch_stable_elapsed_s;
static bool g_vehicle_braking_active;
static bool g_task3_control_profile_active;
static bool g_task3_negative_target_reached;

static float BallBalance_ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float BallBalance_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float BallBalance_MinFloat(float left, float right)
{
    return (left < right) ? left : right;
}

static float BallBalance_MaxFloat(float left, float right)
{
    return (left > right) ? left : right;
}

static float BallBalance_GetActiveMaximumTargetVelocity(void)
{
    float maximum_velocity = BallBalance_AbsFloat(
        g_ball_balance_config.max_target_velocity_mm_s);
    float positive_velocity = BallBalance_AbsFloat(
        g_ball_balance_config.task3_positive_target_velocity_mm_s);

    if ((g_position_gain_scale > 1.05f) &&
        (positive_velocity > 0.001f)) {
        return positive_velocity;
    }
    return maximum_velocity;
}

static float BallBalance_Smoothstep01(float value)
{
    value = BallBalance_ClampFloat(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}

static void BallBalance_ResetVehicleMotion(void)
{
    g_vehicle_motion_initialized = false;
    g_vehicle_previous_measured_speed_cmps = 0.0f;
    g_vehicle_previous_command_speed_cmps = 0.0f;
    g_vehicle_measured_sample_elapsed_s = 0.0f;
    g_vehicle_command_sample_elapsed_s = 0.0f;
    g_vehicle_measured_filter_decay_elapsed_s = 0.0f;
    g_vehicle_command_filter_decay_elapsed_s = 0.0f;
    g_vehicle_measured_acceleration_filter_mm_s2 = 0.0f;
    g_vehicle_command_acceleration_filter_mm_s2 = 0.0f;
    g_vehicle_measured_acceleration_mm_s2 = 0.0f;
    g_vehicle_command_acceleration_mm_s2 = 0.0f;
    g_vehicle_feedforward_acceleration_mm_s2 = 0.0f;
    g_vehicle_feedforward_servo_deg = 0.0f;
    g_vehicle_feedforward_motion_scale = 1.0f;
    g_vehicle_feedforward_motion_sign = 0.0f;
    g_vehicle_filtered_yaw_rate_dps = 0.0f;
    g_vehicle_turn_compensation_servo_deg = 0.0f;
    g_vehicle_launch_armed = true;
    g_vehicle_launch_hold_active = false;
    g_vehicle_launch_hold_servo_deg = 0.0f;
    g_vehicle_launch_motion_direction = 0.0f;
    g_vehicle_launch_elapsed_s = 0.0f;
    g_vehicle_launch_stable_elapsed_s = 0.0f;
}

static float BallBalance_CalculateLaunchCompensation(
    float position_error_mm,
    float target_velocity_mm_s,
    float ball_velocity_mm_s)
{
    float position_threshold = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_error_mm);
    float target_speed = BallBalance_AbsFloat(target_velocity_mm_s);
    float launch_angle = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_servo_max_deg);
    float full_launch_error = BallBalance_AbsFloat(
        g_ball_balance_config.servo_near_target_full_error_mm);
    float direction = (target_velocity_mm_s >= 0.0f) ? 1.0f : -1.0f;
    float release_speed = 0.0f;
    float speed_progress;
    float task3_target_window = BallBalance_AbsFloat(
        g_ball_balance_config.task3_negative_minimum_move_error_mm);
    float task3_anti_decay_ratio = BallBalance_ClampFloat(
        g_ball_balance_config.task3_speed_anti_decay_ratio, 0.0f, 1.0f);
    float task3_anti_decay_speed;
    bool task3_positive_sprint =
        (g_position_gain_scale > 1.05f) &&
        (target_velocity_mm_s > 0.0f);
    bool task3_negative_approach =
        g_task3_control_profile_active &&
        !g_task3_negative_target_reached &&
        (g_target_reference_mm < -0.001f) &&
        (position_error_mm < -task3_target_window) &&
        (target_velocity_mm_s < 0.0f);
    bool task3_speed_approach =
        task3_positive_sprint || task3_negative_approach;

    if (g_minimum_move_active) {
        if (direction < 0.0f) {
            release_speed = BallBalance_AbsFloat(
                g_ball_balance_config.
                    positive_near_minimum_move_release_speed_mm_s);
        } else if (g_negative_return_assist_active) {
            release_speed = BallBalance_AbsFloat(
                g_ball_balance_config.
                    negative_near_minimum_move_release_speed_mm_s);
        }
        target_speed = BallBalance_MaxFloat(target_speed, release_speed);
    }

    if ((BallBalance_AbsFloat(position_error_mm) <= position_threshold) ||
        (target_speed <= 0.001f)) {
        return 0.0f;
    }
    if (!g_minimum_move_active) {
        if (g_ball_balance_config.launch_compensation_requires_stationary ||
            (!task3_speed_approach &&
             (BallBalance_AbsFloat(position_error_mm) < full_launch_error))) {
            return 0.0f;
        }
    }

    if (g_minimum_move_active &&
        (g_minimum_move_servo_angle_deg > 0.001f)) {
        launch_angle = BallBalance_AbsFloat(
            g_minimum_move_servo_angle_deg);
    }
    if (task3_speed_approach && (task3_anti_decay_ratio > 0.001f)) {
        task3_anti_decay_speed =
            BallBalance_GetActiveMaximumTargetVelocity() *
            task3_anti_decay_ratio;
        target_speed = BallBalance_MaxFloat(
            task3_anti_decay_speed, 0.001f);
    }
    speed_progress = BallBalance_ClampFloat(
        direction * ball_velocity_mm_s / target_speed, 0.0f, 1.0f);

    /* Task 3 fades this boost to zero at 80% of the route speed. If terrain
     * cancels ball motion before the scoring window, the same continuous
     * speed feedback restores the available angle without a stall timer. */
    return direction * launch_angle *
        (1.0f - BallBalance_Smoothstep01(speed_progress));
}

static float BallBalance_CalculateServoAngleLimit(
    float normal_limit_deg,
    float position_error_mm,
    float ball_velocity_mm_s,
    float control_servo_offset_deg)
{
    float minimum_limit = BallBalance_MinFloat(
        BallBalance_AbsFloat(
            g_ball_balance_config.servo_near_target_min_limit_deg),
        normal_limit_deg);
    float full_error = BallBalance_AbsFloat(
        g_ball_balance_config.servo_near_target_full_error_mm);
    float full_brake_velocity = BallBalance_AbsFloat(
        g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s);
    float task3_window = BallBalance_AbsFloat(
        g_ball_balance_config.task3_negative_minimum_move_error_mm);
    float task3_speed_ratio = BallBalance_ClampFloat(
        g_ball_balance_config.task3_speed_anti_decay_ratio, 0.0f, 1.0f);
    float task3_route_speed = 0.0f;
    float task3_speed_toward_target = 0.0f;
    float urgency = 0.0f;
    bool task3_positive_approach =
        (g_position_gain_scale > 1.05f) &&
        (position_error_mm > task3_window);
    bool task3_negative_approach =
        g_task3_control_profile_active &&
        !g_task3_negative_target_reached &&
        (g_target_reference_mm < -0.001f) &&
        (position_error_mm < -task3_window);

    /* Only task 3's positive leg uses the higher breakaway limit.  The
     * positive gain scale is set by subtask.c for 0 -> +5 cm; the negative
     * leg keeps its proven conservative limit. */
    if ((g_position_gain_scale > 1.05f) &&
        (position_error_mm > 0.0f) &&
        (g_ball_balance_config.task3_positive_servo_limit_deg > 0.0f)) {
        normal_limit_deg = BallBalance_MinFloat(
            BallBalance_AbsFloat(
                g_ball_balance_config.task3_positive_servo_limit_deg),
            BALL_SERVO_MAX_ANGLE_DEG);
    }

    if (g_minimum_move_active) {
        float negative_near_error_max = BallBalance_AbsFloat(
            g_ball_balance_config.negative_near_minimum_move_error_max_mm);
        float negative_near_angle_limit = BallBalance_AbsFloat(
            g_ball_balance_config.negative_near_minimum_move_servo_max_deg);

        if ((position_error_mm > 0.0f) &&
            (negative_near_error_max > 0.001f) &&
            (position_error_mm <= negative_near_error_max) &&
            (negative_near_angle_limit > 0.001f)) {
            return BallBalance_MinFloat(
                normal_limit_deg,
                BallBalance_MinFloat(
                    negative_near_angle_limit,
                    BALL_SERVO_MAX_ANGLE_DEG));
        }
        /* The breakaway ramp is the only case allowed to exceed the normal
         * task-3 positive limit. Once motion is detected the caller clears
         * this state and the smaller normal limit is restored. */
        return BallBalance_MaxFloat(
            normal_limit_deg,
            BallBalance_MinFloat(
                BallBalance_AbsFloat(g_minimum_move_servo_angle_deg),
                BALL_SERVO_MAX_ANGLE_DEG));
    }
    if (task3_positive_approach || task3_negative_approach) {
        task3_route_speed = task3_positive_approach ?
            BallBalance_AbsFloat(
                g_ball_balance_config.task3_positive_target_velocity_mm_s) :
            BallBalance_AbsFloat(
                g_ball_balance_config.max_target_velocity_mm_s);
        task3_speed_toward_target = task3_positive_approach ?
            ball_velocity_mm_s : -ball_velocity_mm_s;
        if ((task3_route_speed > 0.001f) &&
            (task3_speed_ratio > 0.001f) &&
            (task3_speed_toward_target <
             task3_speed_ratio * task3_route_speed)) {
            /* Terrain cancelled the route speed in the latest tests while
             * the distance envelope held the servo at only 13--17 degrees.
             * Restore the task limit until speed recovers or the ball enters
             * the accepted target window. */
            return normal_limit_deg;
        }
    }
    if (full_error > 0.001f) {
        urgency = BallBalance_Smoothstep01(
            BallBalance_AbsFloat(position_error_mm) / full_error);
    }
    if (((control_servo_offset_deg * ball_velocity_mm_s) < 0.0f) &&
        (full_brake_velocity > 0.001f)) {
        urgency = BallBalance_MaxFloat(
            urgency,
            BallBalance_Smoothstep01(
                BallBalance_AbsFloat(ball_velocity_mm_s) /
                full_brake_velocity));
    }

    return minimum_limit +
        (normal_limit_deg - minimum_limit) * urgency;
}

static float BallBalance_EffectiveRollingGravity(void)
{
    return BallBalance_AbsFloat(g_ball_balance_config.gravity_mm_s2) *
           BallBalance_ClampFloat(
               BallBalance_AbsFloat(
                   g_ball_balance_config.rolling_acceleration_ratio),
               0.01f, 1.0f);
}

static float BallBalance_AccelerationToBeamAngle(float acceleration_mm_s2)
{
    float rolling_gravity = BallBalance_EffectiveRollingGravity();
    float sine_angle;

    if (rolling_gravity <= 0.001f) {
        return 0.0f;
    }
    sine_angle = BallBalance_ClampFloat(
        acceleration_mm_s2 / rolling_gravity, -0.999f, 0.999f);
    return asinf(sine_angle) * BALL_RAD_TO_DEG;
}

static float BallBalance_ServoOffsetToRackTravel(float servo_offset_deg)
{
    return BallBalance_AbsFloat(
               g_ball_balance_config.servo_gear_radius_mm) *
           servo_offset_deg * BALL_DEG_TO_RAD;
}

static float BallBalance_CalculateBreakawayAcceleration(void)
{
    float beam_length = BallBalance_AbsFloat(
        g_ball_balance_config.beam_length_mm);
    float breakaway_travel = BallBalance_AbsFloat(
        g_ball_balance_config.breakaway_rack_travel_mm);
    float margin = BallBalance_MaxFloat(
        g_ball_balance_config.breakaway_acceleration_margin, 1.0f);

    if (beam_length <= 0.001f) {
        return 0.0f;
    }
    return BallBalance_EffectiveRollingGravity() *
           BallBalance_ClampFloat(
               breakaway_travel / beam_length, 0.0f, 0.999f) *
           margin;
}

static void BallBalance_ResetMinimumMove(void)
{
    g_minimum_move_anchor_position_mm = 0.0f;
    g_minimum_move_acceleration_mm_s2 = 0.0f;
    g_minimum_move_servo_angle_deg = 0.0f;
    g_minimum_move_direction = 0.0f;
    g_minimum_move_anchor_received_ms = 0U;
    g_minimum_move_elapsed_ms = 0U;
    g_minimum_move_active_started_ms = 0U;
    g_minimum_move_cooldown_started_ms = 0U;
    g_minimum_move_observation_active = false;
    g_minimum_move_active = false;
    g_minimum_move_cooldown_active = false;
}

static void BallBalance_StartMinimumMoveCooldown(uint32_t now_ms)
{
    g_minimum_move_anchor_position_mm = 0.0f;
    g_minimum_move_acceleration_mm_s2 = 0.0f;
    g_minimum_move_servo_angle_deg = 0.0f;
    g_minimum_move_direction = 0.0f;
    g_minimum_move_anchor_received_ms = 0U;
    g_minimum_move_elapsed_ms = 0U;
    g_minimum_move_active_started_ms = 0U;
    g_minimum_move_observation_active = false;
    g_minimum_move_active = false;
    g_minimum_move_cooldown_started_ms = now_ms;
    g_minimum_move_cooldown_active = true;
}

static void BallBalance_ResetMotionProfile(void)
{
    g_motion_direction = 0.0f;
    g_profile_peak_velocity_mm_s = 0.0f;
    g_profile_braking_velocity_mm_s = 0.0f;
    g_motion_profile_initialized = false;
    g_motion_phase = BALL_BALANCE_MOTION_HOLD;
}

static void BallBalance_ResetCascadeState(void)
{
    g_target_velocity_mm_s = 0.0f;
    g_velocity_integral_servo_deg = 0.0f;
    g_filtered_acceleration_mm_s2 = 0.0f;
    g_previous_acceleration_velocity_mm_s = 0.0f;
    g_previous_acceleration_timestamp_ms = 0U;
    g_acceleration_filter_initialized = false;
    g_negative_return_assist_active = false;
    BallBalance_ResetMotionProfile();
    BallBalance_ResetMinimumMove();
}

static float BallBalance_UpdateMinimumMove(
    const PublishedVisionSample *sample,
    bool new_sample,
    bool measured,
    float position_error_mm,
    float target_acceleration_mm_s2,
    uint32_t now_ms)
{
    float error_threshold = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_error_mm);
    float stationary_threshold = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_stationary_delta_mm);
    float release_speed = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_release_speed_mm_s);
    float minimum_acceleration = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_acceleration_mm_s2);
    float breakaway_acceleration =
        BallBalance_CalculateBreakawayAcceleration();
    float maximum_acceleration = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_acceleration_max_mm_s2);
    float ramp_rate = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_move_acceleration_ramp_mm_s3);
    float negative_near_error_max = BallBalance_AbsFloat(
        g_ball_balance_config.negative_near_minimum_move_error_max_mm);
    float positive_near_error_max = BallBalance_AbsFloat(
        g_ball_balance_config.positive_near_minimum_move_error_max_mm);
    float launch_compensation_direction =
        ((g_ball_balance_status.vehicle_command_speed_cmps < 0.0f) ?
            -1.0f : 1.0f) *
        ((g_ball_balance_config.vehicle_feedforward_direction < 0) ?
            -1.0f : 1.0f);
    float motion_delta;
    float motion_toward_target;
    uint32_t active_elapsed_ms;
    bool negative_near_correction =
        (position_error_mm > 0.0f) &&
        (negative_near_error_max > 0.001f) &&
        (position_error_mm <= negative_near_error_max);
    bool positive_near_correction =
        (position_error_mm < 0.0f) &&
        (positive_near_error_max > 0.001f) &&
        (-position_error_mm <= positive_near_error_max);
    bool negative_return_requires_speed =
        negative_near_correction && g_negative_return_assist_active;
    bool task3_negative_target =
        g_task3_control_profile_active &&
        (g_target_reference_mm < -0.001f);
    bool near_target_requires_speed =
        negative_return_requires_speed || positive_near_correction;
    bool target_hold_requires_confirmed_motion =
        g_target_hold_tune_enabled && near_target_requires_speed;
    bool target_hold_turning =
        g_target_hold_tune_enabled && g_vehicle_feedforward_enabled &&
        (BallBalance_AbsFloat(g_vehicle_filtered_yaw_rate_dps) >=
         BALL_TARGET_HOLD_TURN_YAW_RATE_DPS);
    bool vehicle_launch_pending =
        g_vehicle_feedforward_enabled &&
        (BallBalance_AbsFloat(
            g_ball_balance_status.vehicle_command_speed_cmps) >=
         BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_launch_detect_speed_cmps)) &&
        (BallBalance_AbsFloat(
            g_ball_balance_status.vehicle_measured_speed_cmps) < 1.0f);
    bool minimum_move_aids_launch =
        g_minimum_move_active &&
        ((g_minimum_move_direction *
          ((BallBalance_AbsFloat(
                g_vehicle_feedforward_acceleration_mm_s2) > 0.001f) ?
              g_vehicle_feedforward_acceleration_mm_s2 :
               launch_compensation_direction)) > 0.0f);

    /* Before the task-3 start key is pressed, center the ball only with the
     * cascade. Repeated breakaway pulses in the accepted center window made
     * the stationary beam oscillate before timing had even started. */
    if (g_task3_control_profile_active &&
        (BallBalance_AbsFloat(g_target_reference_mm) < 0.001f)) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    /* Task 3 treats +5 cm as a pass-through turnaround point. Its positive
     * leg must remain under cascade control instead of entering the static
     * friction pulse state, which previously stopped near +4 cm and restarted. */
    if (g_position_gain_scale > 1.05f) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    if (task3_negative_target && g_task3_negative_target_reached) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    /* Once the task-3 negative target is inside its +/-1 cm scoring window,
     * keep only the cascade hold. A 27--30 degree breakaway pulse here caused
     * the measured -6.0 to -4.2 cm repeated oscillation after arrival. */
    if (task3_negative_target) {
        error_threshold = BallBalance_MaxFloat(
            error_threshold,
            BallBalance_AbsFloat(
                g_ball_balance_config.
                    task3_negative_minimum_move_error_mm));
    }

    /* The task-7 standard target already satisfies the scoring tolerance
     * inside +/-1 cm. Do not re-enter the 26--38 degree breakaway pulse in
     * that accepted window; the fixed hold bias and cascade remain active. */
    if (g_target_hold_tune_enabled) {
        error_threshold = BallBalance_MaxFloat(
            error_threshold, BALL_TARGET_HOLD_MINIMUM_MOVE_ERROR_MM);
    }

    /* Chassis vibration and centripetal acceleration already keep the ball
     * moving in a curve. A breakaway pulse here changed a temporary low-speed
     * sample into a measured +9.3 to +4.2 cm overshoot. Let the cascade keep
     * controlling and restore breakaway automatically after the curve. */
    if (target_hold_turning) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    /* A stored breakaway angle in the launch-compensation direction already
     * counters chassis inertia. Preserve it until the ball moves; clearing
     * it caused a measured -30 to -7.6 degree step exactly as the wheels
     * started. A correction in the opposite direction is still discarded. */
    if (vehicle_launch_pending && !minimum_move_aids_launch) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    if (negative_near_correction) {
        release_speed = BallBalance_AbsFloat(
            g_ball_balance_config.
                negative_near_minimum_move_release_speed_mm_s);
    } else if (positive_near_correction) {
        release_speed = BallBalance_AbsFloat(
            g_ball_balance_config.
                positive_near_minimum_move_release_speed_mm_s);
    }
    if (target_hold_requires_confirmed_motion) {
        release_speed = BallBalance_MinFloat(release_speed, 15.0f);
    }

    if (minimum_acceleration <= 0.001f) {
        minimum_acceleration = breakaway_acceleration;
    }

    if (maximum_acceleration < minimum_acceleration) {
        float swap = minimum_acceleration;
        minimum_acceleration = maximum_acceleration;
        maximum_acceleration = swap;
    }
    if ((BallBalance_AbsFloat(position_error_mm) <= error_threshold) ||
        (g_minimum_move_active &&
         ((g_minimum_move_direction * position_error_mm) <= 0.0f))) {
        BallBalance_ResetMinimumMove();
        return target_acceleration_mm_s2;
    }

    if (g_minimum_move_active) {
        float servo_start_angle = BallBalance_AbsFloat(
            g_ball_balance_config.minimum_move_servo_start_deg);
        float servo_max_angle = BallBalance_AbsFloat(
            g_ball_balance_config.minimum_move_servo_max_deg);
        float servo_ramp_rate = BallBalance_AbsFloat(
            g_ball_balance_config.minimum_move_servo_ramp_deg_s);

        if (negative_near_correction) {
            servo_start_angle = BallBalance_AbsFloat(
                g_ball_balance_config.
                    negative_near_minimum_move_servo_start_deg);
            servo_max_angle = BallBalance_AbsFloat(
                g_ball_balance_config.
                    negative_near_minimum_move_servo_max_deg);
            servo_ramp_rate = BallBalance_AbsFloat(
                g_ball_balance_config.
                    negative_near_minimum_move_servo_ramp_deg_s);
        }

        active_elapsed_ms = (uint32_t)(
            now_ms - g_minimum_move_active_started_ms);
        g_minimum_move_elapsed_ms =
            (uint32_t)g_ball_balance_config.minimum_move_detect_ms +
            active_elapsed_ms;

        if (new_sample && measured) {
            bool release_ready;

            motion_toward_target = g_minimum_move_direction *
                ((float)sample->position_mm -
                 g_minimum_move_anchor_position_mm);
            if (target_hold_requires_confirmed_motion) {
                release_ready =
                    (motion_toward_target > stationary_threshold) &&
                    ((g_minimum_move_direction *
                      g_filtered_velocity_mm_s) >= release_speed);
            } else {
                release_ready =
                    (!near_target_requires_speed &&
                     (motion_toward_target > stationary_threshold)) ||
                    ((g_minimum_move_direction *
                      g_filtered_velocity_mm_s) >= release_speed);
            }
            if (release_ready) {
                BallBalance_StartMinimumMoveCooldown(now_ms);
                return target_acceleration_mm_s2;
            }
        }

        g_minimum_move_acceleration_mm_s2 = BallBalance_ClampFloat(
            minimum_acceleration + ramp_rate *
                ((float)active_elapsed_ms / 1000.0f),
            minimum_acceleration, maximum_acceleration);
        if (servo_max_angle < servo_start_angle) {
            float swap = servo_start_angle;
            servo_start_angle = servo_max_angle;
            servo_max_angle = swap;
        }
        g_minimum_move_servo_angle_deg = BallBalance_ClampFloat(
            servo_start_angle + servo_ramp_rate *
                ((float)active_elapsed_ms / 1000.0f),
            servo_start_angle, servo_max_angle);
        if ((target_acceleration_mm_s2 * g_minimum_move_direction) <
            g_minimum_move_acceleration_mm_s2) {
            return g_minimum_move_direction *
                   g_minimum_move_acceleration_mm_s2;
        }
        return target_acceleration_mm_s2;
    }

    if (g_minimum_move_cooldown_active) {
        if ((uint32_t)(now_ms - g_minimum_move_cooldown_started_ms) <
            (uint32_t)g_ball_balance_config.minimum_move_cooldown_ms) {
            return target_acceleration_mm_s2;
        }
        g_minimum_move_cooldown_active = false;
    }

    if (!new_sample) {
        return target_acceleration_mm_s2;
    }
    if (!measured) {
        g_minimum_move_observation_active = false;
        g_minimum_move_elapsed_ms = 0U;
        return target_acceleration_mm_s2;
    }
    if (BallBalance_AbsFloat(g_filtered_velocity_mm_s) >= release_speed) {
        g_minimum_move_observation_active = false;
        g_minimum_move_elapsed_ms = 0U;
        return target_acceleration_mm_s2;
    }

    if (!g_minimum_move_observation_active) {
        g_minimum_move_anchor_position_mm = (float)sample->position_mm;
        g_minimum_move_anchor_received_ms = sample->received_at_ms;
        g_minimum_move_observation_active = true;
        return target_acceleration_mm_s2;
    }

    motion_delta = BallBalance_AbsFloat(
        (float)sample->position_mm - g_minimum_move_anchor_position_mm);
    if (motion_delta > stationary_threshold) {
        g_minimum_move_anchor_position_mm = (float)sample->position_mm;
        g_minimum_move_anchor_received_ms = sample->received_at_ms;
        g_minimum_move_elapsed_ms = 0U;
        return target_acceleration_mm_s2;
    }

    g_minimum_move_elapsed_ms = (uint32_t)(
        sample->received_at_ms - g_minimum_move_anchor_received_ms);
    if (g_minimum_move_elapsed_ms <
        (uint32_t)g_ball_balance_config.minimum_move_detect_ms) {
        return target_acceleration_mm_s2;
    }

    g_minimum_move_anchor_position_mm = (float)sample->position_mm;
    g_minimum_move_direction =
        (position_error_mm >= 0.0f) ? 1.0f : -1.0f;
    g_minimum_move_acceleration_mm_s2 = minimum_acceleration;
    g_minimum_move_servo_angle_deg = BallBalance_AbsFloat(
        negative_near_correction ?
            g_ball_balance_config.
                negative_near_minimum_move_servo_start_deg :
            g_ball_balance_config.minimum_move_servo_start_deg);
    g_minimum_move_active_started_ms = now_ms;
    g_minimum_move_active = true;
    if ((target_acceleration_mm_s2 * g_minimum_move_direction) <
        g_minimum_move_acceleration_mm_s2) {
        return g_minimum_move_direction *
               g_minimum_move_acceleration_mm_s2;
    }
    return target_acceleration_mm_s2;
}

static void BallBalance_UpdateVelocityIntegral(
    bool controller_active,
    bool measured,
    bool velocity_limit_active,
    float velocity_error_mm_s,
    float corrected_acceleration_mm_s2,
    float ball_velocity_mm_s,
    float launch_compensation_servo_deg)
{
    float error_deadband = BallBalance_AbsFloat(
        g_ball_balance_config.velocity_integral_deadband_mm_s);
    float integral_limit = BallBalance_AbsFloat(
        g_ball_balance_config.velocity_integral_limit_deg);
    float integral_gain;

    if (!controller_active) {
        g_velocity_integral_servo_deg = 0.0f;
        return;
    }

    if (measured &&
        (BallBalance_AbsFloat(velocity_error_mm_s) > error_deadband) &&
        (((g_velocity_integral_servo_deg * velocity_error_mm_s) < 0.0f) ||
         (BallBalance_AbsFloat(launch_compensation_servo_deg) < 1.0f))) {
        if ((g_velocity_integral_servo_deg * velocity_error_mm_s) < 0.0f) {
            integral_gain = BallBalance_AbsFloat(
                g_ball_balance_config.velocity_integral_unwind_gain_deg_per_mm);
        } else {
            integral_gain = BallBalance_AbsFloat(
                g_ball_balance_config.velocity_integral_gain_deg_per_mm);
        }
        g_velocity_integral_servo_deg += integral_gain *
            velocity_error_mm_s * BALL_CONTROL_PERIOD_S;
    }

    /* An old drive bias must not mask an emergency braking command. The
     * command-angle slew still makes the physical reversal continuous. */
    if (velocity_limit_active &&
        ((corrected_acceleration_mm_s2 * ball_velocity_mm_s) < 0.0f) &&
        ((g_velocity_integral_servo_deg *
          corrected_acceleration_mm_s2) < 0.0f)) {
        g_velocity_integral_servo_deg = 0.0f;
    }

    g_velocity_integral_servo_deg = BallBalance_ClampFloat(
        g_velocity_integral_servo_deg, -integral_limit, integral_limit);
}

static void BallBalance_UpdateAccelerationEstimate(
    const PublishedVisionSample *sample,
    bool new_sample,
    bool measured)
{
    uint32_t delta_ms;
    float raw_acceleration;
    float alpha;

    if (!new_sample || !measured) {
        return;
    }

    if (!g_acceleration_filter_initialized) {
        g_previous_acceleration_velocity_mm_s =
            g_filtered_velocity_mm_s;
        g_previous_acceleration_timestamp_ms = sample->timestamp_ms;
        g_filtered_acceleration_mm_s2 = 0.0f;
        g_acceleration_filter_initialized = true;
        return;
    }

    delta_ms = (uint32_t)(sample->timestamp_ms -
                          g_previous_acceleration_timestamp_ms);
    if ((delta_ms >= BALL_ACCELERATION_DT_MIN_MS) &&
        (delta_ms <= BALL_ACCELERATION_DT_MAX_MS)) {
        raw_acceleration =
            (g_filtered_velocity_mm_s -
             g_previous_acceleration_velocity_mm_s) *
            (1000.0f / (float)delta_ms);
        raw_acceleration = BallBalance_ClampFloat(
            raw_acceleration,
            -BALL_ACCELERATION_PHYSICAL_LIMIT_MM_S2,
            BALL_ACCELERATION_PHYSICAL_LIMIT_MM_S2);
        alpha = BallBalance_ClampFloat(
            g_ball_balance_config.acceleration_filter_alpha,
            0.0f, 1.0f);
        g_filtered_acceleration_mm_s2 += alpha *
            (raw_acceleration - g_filtered_acceleration_mm_s2);
    } else {
        g_filtered_acceleration_mm_s2 = 0.0f;
    }

    g_previous_acceleration_velocity_mm_s =
        g_filtered_velocity_mm_s;
    g_previous_acceleration_timestamp_ms = sample->timestamp_ms;
}

/* Online trapezoidal profile. The peak speed is fixed from the distance at
 * profile start; the braking envelope is recomputed from remaining distance. */
static float BallBalance_CalculateProfileVelocity(
    float position_error_mm,
    float ball_velocity_mm_s)
{
    float absolute_error = BallBalance_AbsFloat(position_error_mm);
    float hold_tolerance = BallBalance_AbsFloat(
        g_ball_balance_config.settle_position_tolerance_mm);
    float travel_distance = BallBalance_MaxFloat(
        absolute_error - hold_tolerance, 0.0f);
    float maximum_velocity =
        BallBalance_GetActiveMaximumTargetVelocity();
    float minimum_velocity = BallBalance_MinFloat(
        BallBalance_AbsFloat(
            g_ball_balance_config.minimum_target_velocity_mm_s),
        maximum_velocity);
    float minimum_velocity_full_error = BallBalance_AbsFloat(
        g_ball_balance_config.minimum_target_velocity_full_error_mm);
    float acceleration = BallBalance_AbsFloat(
        g_ball_balance_config.velocity_reference_accel_limit_mm_s2) *
        BallBalance_ClampFloat(
            g_position_gain_scale,
            BALL_POSITION_GAIN_SCALE_MIN,
            BALL_POSITION_GAIN_SCALE_MAX);
    float braking_acceleration = BallBalance_AbsFloat(
        g_ball_balance_config.braking_acceleration_mm_s2);
    float phase_hysteresis = BallBalance_AbsFloat(
        g_ball_balance_config.motion_phase_velocity_hysteresis_mm_s);
    float direction = (position_error_mm >= 0.0f) ? 1.0f : -1.0f;
    float velocity_toward_target = direction * ball_velocity_mm_s;
    float prediction_time = BallBalance_ClampFloat(
        g_ball_balance_config.motion_prediction_time_s, 0.0f, 1.0f);
    float reaction_distance;
    float braking_travel_distance;
    float initial_velocity_toward_target;
    float triangular_peak_squared;
    float distance_based_peak;
    float requested_speed;
    float task3_target_window = BallBalance_AbsFloat(
        g_ball_balance_config.task3_negative_minimum_move_error_mm);
    float task3_speed_floor = maximum_velocity * BallBalance_ClampFloat(
        g_ball_balance_config.task3_speed_anti_decay_ratio, 0.0f, 1.0f);
    bool task3_negative_target =
        g_task3_control_profile_active &&
        (g_target_reference_mm < -0.001f);

    /* Task 3's positive point is a turnaround, not a stopping target. Keep
     * requesting the configured maximum speed until subtask.c observes
     * +4.5 cm and changes the target to -5.5 cm. The normal speed limiter
     * remains active, so this does not permit measured overspeed. */
    if ((g_position_gain_scale > 1.05f) &&
        (position_error_mm > 0.0f) &&
        (maximum_velocity > 0.001f)) {
        g_motion_direction = 1.0f;
        g_profile_peak_velocity_mm_s = maximum_velocity;
        g_profile_braking_velocity_mm_s = maximum_velocity;
        g_motion_profile_initialized = true;
        g_motion_phase = BALL_BALANCE_MOTION_CRUISE;
        return maximum_velocity;
    }

    /* Task 3 accepts the negative endpoint anywhere inside +/-1 cm. Stop the
     * velocity reference at that boundary instead of continuing toward the
     * compensated -5.5 cm center and then firing another correction. */
    if (task3_negative_target &&
        (absolute_error <= task3_target_window)) {
        g_task3_negative_target_reached = true;
        g_motion_direction = direction;
        g_profile_peak_velocity_mm_s = 0.0f;
        g_profile_braking_velocity_mm_s = 0.0f;
        g_motion_profile_initialized = false;
        g_motion_phase = BALL_BALANCE_MOTION_HOLD;
        return 0.0f;
    }

    /* Other phases use a conventional stop-at-target braking envelope. */
    braking_acceleration *= BallBalance_ClampFloat(
        g_position_gain_scale,
        BALL_POSITION_GAIN_SCALE_MIN,
        BALL_POSITION_GAIN_SCALE_MAX);

    if ((travel_distance <= 0.0f) ||
        (maximum_velocity <= 0.001f) ||
        (acceleration <= 0.001f) ||
        (braking_acceleration <= 0.001f)) {
        g_motion_direction = direction;
        g_profile_peak_velocity_mm_s = 0.0f;
        g_profile_braking_velocity_mm_s = 0.0f;
        g_motion_profile_initialized = false;
        g_motion_phase = BALL_BALANCE_MOTION_HOLD;
        return 0.0f;
    }

    if (minimum_velocity_full_error > 0.001f) {
        minimum_velocity *= BallBalance_Smoothstep01(
            travel_distance / minimum_velocity_full_error);
    }

    if (!g_motion_profile_initialized ||
        ((direction * g_motion_direction) <= 0.0f)) {
        initial_velocity_toward_target = BallBalance_MaxFloat(
            velocity_toward_target, 0.0f);
        triangular_peak_squared =
            (2.0f * acceleration * braking_acceleration * travel_distance +
             braking_acceleration * initial_velocity_toward_target *
                 initial_velocity_toward_target) /
            (acceleration + braking_acceleration);
        distance_based_peak = BallBalance_AbsFloat(
            g_ball_balance_config.position_to_velocity_kp_s) *
            g_position_gain_scale * travel_distance;
        g_profile_peak_velocity_mm_s = BallBalance_MinFloat(
            maximum_velocity,
            BallBalance_MinFloat(
                distance_based_peak,
                sqrtf(BallBalance_MaxFloat(
                    triangular_peak_squared, 0.0f))));
        g_motion_direction = direction;
        g_motion_profile_initialized = true;
        g_motion_phase = BALL_BALANCE_MOTION_ACCELERATE;
    }

    reaction_distance = BallBalance_MaxFloat(
        velocity_toward_target, 0.0f) * prediction_time;
    braking_travel_distance = BallBalance_MaxFloat(
        travel_distance - reaction_distance, 0.0f);
    g_profile_braking_velocity_mm_s = sqrtf(
        2.0f * braking_acceleration * braking_travel_distance);
    requested_speed = BallBalance_MinFloat(
        g_profile_peak_velocity_mm_s,
        g_profile_braking_velocity_mm_s);

    /* Outside the position window, keep a small creep reference after the
     * ball has nearly stopped. This prevents the delay allowance from
     * intentionally parking the ball just short of the target. */
    if ((requested_speed < minimum_velocity) &&
        (velocity_toward_target <= minimum_velocity)) {
        requested_speed = minimum_velocity;
    }
    if (task3_negative_target &&
        !g_task3_negative_target_reached &&
        (position_error_mm < -task3_target_window)) {
        requested_speed = BallBalance_MaxFloat(
            requested_speed, task3_speed_floor);
    }

    if (((requested_speed + phase_hysteresis) <
         g_profile_peak_velocity_mm_s) ||
        (velocity_toward_target >
         (requested_speed + phase_hysteresis))) {
        g_motion_phase = BALL_BALANCE_MOTION_DECELERATE;
    } else if ((velocity_toward_target + phase_hysteresis) <
               g_profile_peak_velocity_mm_s) {
        g_motion_phase = BALL_BALANCE_MOTION_ACCELERATE;
    } else {
        g_motion_phase = BALL_BALANCE_MOTION_CRUISE;
    }

    return direction * requested_speed;
}

static float BallBalance_CalculateUrgency(
    float position_error_mm,
    float ball_velocity_mm_s,
    float velocity_error_mm_s,
    float servo_angle_delta_deg)
{
    float error_scale = BallBalance_AbsFloat(
        g_ball_balance_config.servo_speed_full_error_mm);
    float velocity_scale = BallBalance_AbsFloat(
        g_ball_balance_config.servo_speed_full_velocity_mm_s);
    float angle_scale = BallBalance_AbsFloat(
        g_ball_balance_config.servo_speed_full_angle_delta_deg);
    float urgency = 0.0f;

    if (error_scale > 0.001f) {
        urgency = BallBalance_MaxFloat(
            urgency, BallBalance_AbsFloat(position_error_mm) / error_scale);
    }
    if (velocity_scale > 0.001f) {
        urgency = BallBalance_MaxFloat(
            urgency, BallBalance_AbsFloat(ball_velocity_mm_s) /
                     velocity_scale);
        urgency = BallBalance_MaxFloat(
            urgency, BallBalance_AbsFloat(velocity_error_mm_s) /
                     velocity_scale);
    }
    if (angle_scale > 0.001f) {
        urgency = BallBalance_MaxFloat(
            urgency, BallBalance_AbsFloat(servo_angle_delta_deg) /
                     angle_scale);
    }
    return BallBalance_ClampFloat(urgency, 0.0f, 1.0f);
}

static uint16_t BallBalance_CalculateServoSpeed(float urgency)
{
    uint16_t minimum = g_ball_balance_config.servo_speed_min;
    uint16_t maximum = g_ball_balance_config.servo_speed_max;
    uint16_t swap;
    float blend;
    float speed;

    if (minimum == 0U) {
        minimum = 1U;
    }
    if (maximum < minimum) {
        swap = minimum;
        minimum = maximum;
        maximum = swap;
        if (minimum == 0U) {
            minimum = 1U;
        }
    }

    blend = BallBalance_Smoothstep01(urgency);
    speed = (float)minimum +
            ((float)maximum - (float)minimum) * blend;
    return (uint16_t)(speed + 0.5f);
}

static float BallBalance_ClampServoAngle(float angle_deg)
{
    float minimum = BallBalance_ClampFloat(
        g_ball_balance_config.servo_min_angle_deg,
        BALL_SERVO_MIN_ANGLE_DEG, BALL_SERVO_MAX_ANGLE_DEG);
    float maximum = BallBalance_ClampFloat(
        g_ball_balance_config.servo_max_angle_deg,
        BALL_SERVO_MIN_ANGLE_DEG, BALL_SERVO_MAX_ANGLE_DEG);
    float swap;

    if (minimum > maximum) {
        swap = minimum;
        minimum = maximum;
        maximum = swap;
    }

    return BallBalance_ClampFloat(angle_deg, minimum, maximum);
}

static uint16_t Maixcam_ReadU16Le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static int16_t Maixcam_ReadI16Le(const uint8_t *data)
{
    return (int16_t)Maixcam_ReadU16Le(data);
}

static uint32_t Maixcam_ReadU32Le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static uint16_t Maixcam_Crc16(const uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFFU;
    uint8_t index;
    uint8_t bit;

    for (index = 0U; index < length; index++) {
        crc ^= (uint16_t)data[index] << 8U;
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1U) ^ 0x1021U);
            } else {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

/* Keep a partial next-frame header after a corrupt or byte-shifted frame. */
static void Maixcam_ParserRecoverAlignment(MaixcamParser *parser)
{
    uint8_t start;
    uint8_t remaining;

    for (start = 1U; start + 1U < MAIXCAM_PACKET_SIZE; start++) {
        if ((parser->bytes[start] == MAIXCAM_MAGIC_0) &&
            (parser->bytes[start + 1U] == MAIXCAM_MAGIC_1)) {
            remaining = (uint8_t)(MAIXCAM_PACKET_SIZE - start);
            memmove(parser->bytes, &parser->bytes[start], remaining);
            parser->index = remaining;
            return;
        }
    }

    if (parser->bytes[MAIXCAM_PACKET_SIZE - 1U] == MAIXCAM_MAGIC_0) {
        parser->bytes[0] = MAIXCAM_MAGIC_0;
        parser->index = 1U;
    } else {
        parser->index = 0U;
    }
}

static bool Maixcam_ParserPush(MaixcamParser *parser,
                               uint8_t byte,
                               MaixcamPacket *packet)
{
    uint16_t expected_crc;
    uint16_t received_crc;

    if ((parser->index == 0U) && (byte != MAIXCAM_MAGIC_0)) {
        return false;
    }

    if ((parser->index == 1U) && (byte != MAIXCAM_MAGIC_1)) {
        parser->bytes[0] = byte;
        parser->index = (byte == MAIXCAM_MAGIC_0) ? 1U : 0U;
        return false;
    }

    parser->bytes[parser->index] = byte;
    parser->index++;
    if (parser->index < MAIXCAM_PACKET_SIZE) {
        return false;
    }

    parser->index = 0U;
    if (parser->bytes[MAIXCAM_OFFSET_VERSION] !=
        MAIXCAM_PROTOCOL_VERSION) {
        parser->format_errors++;
        Maixcam_ParserRecoverAlignment(parser);
        return false;
    }

    expected_crc = Maixcam_Crc16(parser->bytes, MAIXCAM_CRC_DATA_SIZE);
    received_crc = Maixcam_ReadU16Le(
        &parser->bytes[MAIXCAM_OFFSET_CRC16]);
    if (expected_crc != received_crc) {
        parser->crc_errors++;
        Maixcam_ParserRecoverAlignment(parser);
        return false;
    }

    packet->flags = parser->bytes[MAIXCAM_OFFSET_FLAGS];
    packet->sequence = Maixcam_ReadU16Le(
        &parser->bytes[MAIXCAM_OFFSET_SEQUENCE]);
    packet->timestamp_ms = Maixcam_ReadU32Le(
        &parser->bytes[MAIXCAM_OFFSET_TIMESTAMP_MS]);
    packet->position_mm = Maixcam_ReadI16Le(
        &parser->bytes[MAIXCAM_OFFSET_POSITION_MM]);
    packet->velocity_mm_s = Maixcam_ReadI16Le(
        &parser->bytes[MAIXCAM_OFFSET_VELOCITY_MM_S]);
    packet->confidence_milli = Maixcam_ReadU16Le(
        &parser->bytes[MAIXCAM_OFFSET_CONFIDENCE_MILLI]);
    packet->processing_us = Maixcam_ReadU16Le(
        &parser->bytes[MAIXCAM_OFFSET_PROCESSING_US]);
    parser->valid_frames++;
    return true;
}

static bool Maixcam_ProtocolSelfTest(void)
{
    static const uint8_t test_packet[MAIXCAM_PACKET_SIZE] = {
        0xAAU, 0x55U, 0x01U, 0x03U, 0x34U, 0x12U, 0x40U,
        0xE2U, 0x01U, 0x00U, 0x32U, 0x00U, 0x78U, 0x00U,
        0x84U, 0x03U, 0x68U, 0x10U, 0xFAU, 0x38U
    };
    MaixcamParser parser;
    MaixcamPacket packet;
    uint8_t index;
    bool complete = false;
    bool fields_match;

    memset(&parser, 0, sizeof(parser));
    memset(&packet, 0, sizeof(packet));
    for (index = 0U; index < MAIXCAM_PACKET_SIZE; index++) {
        complete = Maixcam_ParserPush(&parser, test_packet[index], &packet);
    }

    fields_match = complete &&
        (packet.flags == 0x03U) &&
        (packet.sequence == 0x1234U) &&
        (packet.timestamp_ms == 123456U) &&
        (packet.position_mm == 50) &&
        (packet.velocity_mm_s == 120) &&
        (packet.confidence_milli == 900U) &&
        (packet.processing_us == 4200U);
    if (!fields_match) {
        return false;
    }

    /* A truncated frame followed by a complete frame must realign. */
    memset(&parser, 0, sizeof(parser));
    complete = false;
    for (index = 0U; index < MAIXCAM_PACKET_SIZE - 1U; index++) {
        (void)Maixcam_ParserPush(&parser, test_packet[index], &packet);
    }
    for (index = 0U; index < MAIXCAM_PACKET_SIZE; index++) {
        complete = Maixcam_ParserPush(
            &parser, test_packet[index], &packet);
    }

    return complete && (packet.sequence == 0x1234U);
}

static bool BallBalance_SequenceIsNewer(uint16_t current, uint16_t previous)
{
    return (int16_t)(current - previous) > 0;
}

static bool BallBalance_ReadSnapshot(PublishedVisionSample *sample,
                                     uint32_t *revision)
{
    uint32_t before;
    uint32_t after;

    for (;;) {
        before = g_publish_revision;
        if ((before & 1U) != 0U) {
            continue;
        }

        __DMB();
        sample->present = g_published_sample.present;
        sample->flags = g_published_sample.flags;
        sample->sequence = g_published_sample.sequence;
        sample->timestamp_ms = g_published_sample.timestamp_ms;
        sample->position_mm = g_published_sample.position_mm;
        sample->velocity_mm_s = g_published_sample.velocity_mm_s;
        sample->confidence_milli = g_published_sample.confidence_milli;
        sample->processing_us = g_published_sample.processing_us;
        sample->received_at_ms = g_published_sample.received_at_ms;
        __DMB();
        after = g_publish_revision;
        if ((before == after) && ((after & 1U) == 0U)) {
            break;
        }
    }

    *revision = after;
    return sample->present != 0U;
}

static void BallBalance_UpdateTargetReference(void)
{
    float requested_target;

    requested_target = BallBalance_ClampFloat(
        g_ball_balance_target_mm,
        -BALL_TARGET_LIMIT_MM,
        BALL_TARGET_LIMIT_MM);
    g_target_reference_mm = requested_target;

    g_ball_balance_status.requested_target_mm = requested_target;
    g_ball_balance_status.target_reference_mm = g_target_reference_mm;
}

static void BallBalance_ApplyConservativeTune(void)
{
    /* Stationary profile: keep the proven task-3 +/-5 cm behavior. */
    g_ball_balance_config.position_to_velocity_kp_s = 1.25f;
    g_ball_balance_config.max_target_velocity_mm_s = 70.0f;
    g_ball_balance_config.minimum_target_velocity_mm_s = 30.0f;
    /* Fade the approach speed over the last 15 mm instead of dropping it
     * abruptly at the old 3 mm threshold. */
    g_ball_balance_config.minimum_target_velocity_full_error_mm = 15.0f;
    g_ball_balance_config.motion_prediction_time_s = 0.05f;
    g_ball_balance_config.velocity_limit_overspeed_kp_s = 4.0f;
    g_ball_balance_config.velocity_limit_max_brake_mm_s2 = 250.0f;
    g_ball_balance_config.velocity_reference_accel_limit_mm_s2 = 120.0f;
    g_ball_balance_config.braking_acceleration_mm_s2 = 80.0f;
    g_ball_balance_config.distance_brake_gain = 0.0f;
    g_ball_balance_config.distance_brake_stop_offset_mm = 0.0f;
    g_ball_balance_config.velocity_to_acceleration_kp_s = 4.2f;
    g_ball_balance_config.acceleration_limit_near_mm_s2 = 65.0f;
    g_ball_balance_config.acceleration_limit_far_mm_s2 = 130.0f;
    g_ball_balance_config.acceleration_limit_brake_mm_s2 = 250.0f;
    g_ball_balance_config.acceleration_feedback_gain = 0.03f;
    g_ball_balance_config.servo_accel_slew_deg_s = 50.0f;
    g_ball_balance_config.servo_brake_slew_deg_s = 180.0f;
    g_ball_balance_config.velocity_integral_gain_deg_per_mm = 0.07f;
    g_ball_balance_config.servo_hold_bias_deg = 0.0f;
    g_ball_balance_config.velocity_integral_limit_deg = 3.5f;
    g_ball_balance_config.servo_normal_angle_limit_deg = 29.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_deg = 0.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_error_mm = 0.0f;
    g_ball_balance_config.vehicle_braking_feedforward_preload_error_mm = 0.0f;
    g_ball_balance_config.servo_near_target_min_limit_deg = 4.0f;
    g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s = 80.0f;
    g_ball_balance_config.settle_position_tolerance_mm = 5.0f;
    g_ball_balance_config.tracked_gain_scale = 0.50f;
    g_ball_balance_config.minimum_move_error_mm = 9.0f;
    g_ball_balance_config.minimum_move_detect_ms = 150U;
    g_ball_balance_config.minimum_move_cooldown_ms = 250U;
    g_ball_balance_config.launch_compensation_requires_stationary = false;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 5.0f;
    g_ball_balance_config.minimum_move_servo_start_deg = 24.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 27.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 8.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 50.0f;
    g_ball_balance_config.negative_near_minimum_move_error_max_mm = 0.0f;
    g_ball_balance_config.negative_near_minimum_move_release_speed_mm_s = 0.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_start_deg = 0.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_max_deg = 0.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_ramp_deg_s = 0.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_slew_deg_s = 0.0f;
    g_ball_balance_config.positive_near_minimum_move_error_max_mm = 0.0f;
    g_ball_balance_config.positive_near_minimum_move_release_speed_mm_s = 0.0f;
    g_ball_balance_config.negative_return_assist_error_max_mm = 0.0f;
    g_ball_balance_config.negative_return_assist_speed_mm_s = 0.0f;
    g_ball_balance_config.negative_return_assist_release_error_mm = 0.0f;
    g_ball_balance_config.negative_return_assist_brake_limit_mm_s2 = 0.0f;
    g_ball_balance_config.vehicle_braking_vision_timeout_ms = 100U;
    g_ball_balance_config.vehicle_launch_feedforward_slew_deg_s = 0.0f;
    g_ball_balance_config.vehicle_launch_preload_enabled = false;
    g_ball_balance_config.task3_positive_target_velocity_mm_s = 0.0f;
    g_ball_balance_config.task3_negative_minimum_move_error_mm = 0.0f;
    g_ball_balance_config.task3_speed_anti_decay_ratio = 0.0f;
}

static void BallBalance_ApplyTask3Tune(void)
{
    /* The positive leg bypasses minimum-move and cruises through +5 cm.
     * These breakaway settings remain available only for center/negative
     * settling, whose proven behavior must not change. */
    BallBalance_ApplyConservativeTune();
    g_ball_balance_config.velocity_integral_deadband_mm_s = 2.0f;
    g_ball_balance_config.minimum_move_error_mm = 7.0f;
    g_ball_balance_config.minimum_move_stationary_delta_mm = 2.0f;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.minimum_move_acceleration_mm_s2 = 20.0f;
    g_ball_balance_config.minimum_move_acceleration_max_mm_s2 = 100.0f;
    g_ball_balance_config.minimum_move_acceleration_ramp_mm_s3 = 20.0f;
    g_ball_balance_config.minimum_move_servo_start_deg = 27.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 30.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 12.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 80.0f;
    g_ball_balance_config.minimum_move_detect_ms = 100U;
    g_ball_balance_config.minimum_move_cooldown_ms = 150U;
    g_ball_balance_config.task3_positive_servo_limit_deg = 28.0f;
    g_ball_balance_config.task3_positive_target_velocity_mm_s = 80.0f;
    g_ball_balance_config.task3_negative_minimum_move_error_mm = 10.0f;
    g_ball_balance_config.task3_speed_anti_decay_ratio = 0.80f;
}

static void BallBalance_ApplyTask4Tune(void)
{
    /* Independent task-4 moving profile. Keep all values local so higher
     * A-to-B speed tuning cannot change the full-lap task-5 behavior. */
    BallBalance_ApplyConservativeTune();
    g_ball_balance_config.position_to_velocity_kp_s = 1.45f;
    g_ball_balance_config.max_target_velocity_mm_s = 60.0f;
    g_ball_balance_config.minimum_target_velocity_mm_s = 10.0f;
    g_ball_balance_config.minimum_target_velocity_full_error_mm = 15.0f;
    g_ball_balance_config.motion_prediction_time_s = 0.20f;
    g_ball_balance_config.velocity_limit_overspeed_kp_s = 8.0f;
    g_ball_balance_config.velocity_limit_max_brake_mm_s2 = 340.0f;
    g_ball_balance_config.velocity_reference_accel_limit_mm_s2 = 90.0f;
    g_ball_balance_config.braking_acceleration_mm_s2 = 140.0f;
    g_ball_balance_config.distance_brake_gain = 1.00f;
    g_ball_balance_config.distance_brake_stop_offset_mm = 0.0f;
    g_ball_balance_config.velocity_to_acceleration_kp_s = 6.2f;
    g_ball_balance_config.acceleration_limit_near_mm_s2 = 55.0f;
    g_ball_balance_config.acceleration_limit_far_mm_s2 = 120.0f;
    g_ball_balance_config.acceleration_limit_brake_mm_s2 = 340.0f;
    g_ball_balance_config.acceleration_feedback_gain = 0.04f;
    g_ball_balance_config.servo_accel_slew_deg_s = 60.0f;
    g_ball_balance_config.servo_brake_slew_deg_s = 260.0f;
    g_ball_balance_config.velocity_integral_gain_deg_per_mm = 0.10f;
    g_ball_balance_config.velocity_integral_deadband_mm_s = 0.5f;
    g_ball_balance_config.servo_hold_bias_deg = -2.0f;
    g_ball_balance_config.velocity_integral_limit_deg = 3.5f;
    g_ball_balance_config.servo_normal_angle_limit_deg = 32.0f;
    g_ball_balance_config.servo_near_target_min_limit_deg = 5.0f;
    g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s = 50.0f;
    g_ball_balance_config.settle_position_tolerance_mm = 4.0f;
    g_ball_balance_config.tracked_gain_scale = 0.75f;
    g_ball_balance_config.vehicle_feedforward_gain = 1.05f;
    /* Hold a small preload from unlock through the complete 3 s launch ramp.
     * A dedicated slew limit rejects quantized encoder acceleration pulses
     * without slowing the proven task-4 parking feedforward. */
    g_ball_balance_config.vehicle_launch_hold_ratio = 0.36f;
    g_ball_balance_config.vehicle_launch_hold_max_ms = 3300U;
    g_ball_balance_config.vehicle_launch_feedforward_slew_deg_s = 120.0f;
    g_ball_balance_config.vehicle_launch_preload_enabled = true;
    g_ball_balance_config.vehicle_launch_settle_ms = 300U;
    g_ball_balance_config.vehicle_command_acceleration_weight = 0.70f;
    g_ball_balance_config.vehicle_command_acceleration_lead_weight = 0.90f;
    g_ball_balance_config.vehicle_measured_acceleration_takeover_ratio = 0.85f;
    g_ball_balance_config.vehicle_measured_acceleration_filter_alpha = 0.30f;
    g_ball_balance_config.vehicle_command_acceleration_filter_alpha = 0.55f;
    g_ball_balance_config.vehicle_acceleration_deadband_mm_s2 = 80.0f;
    g_ball_balance_config.vehicle_acceleration_limit_mm_s2 = 700.0f;
    g_ball_balance_config.vehicle_feedforward_servo_limit_deg = 23.3f;
    g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s = 327.0f;
    g_ball_balance_config.vehicle_feedforward_release_slew_deg_s = 250.0f;
    g_ball_balance_config.vehicle_feedforward_command_reserve_ratio = 0.50f;
    g_ball_balance_config.vehicle_braking_servo_extra_deg = 4.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_error_mm = 20.0f;
    /* Release braking feedforward earlier when the ball is still moving away
     * on the negative side.  The dynamic exception in Control200Hz keeps the
     * same feedforward once the ball has reversed toward the target. */
    g_ball_balance_config.vehicle_braking_feedforward_preload_error_mm = 12.0f;
    g_ball_balance_config.vehicle_feedforward_direction = 1;
    g_ball_balance_config.vehicle_turn_compensation_gain_deg_per_dps = 0.10f;
    g_ball_balance_config.vehicle_turn_compensation_deadband_dps = 5.0f;
    g_ball_balance_config.vehicle_turn_compensation_limit_deg = 2.9f;
    g_ball_balance_config.vehicle_turn_compensation_filter_alpha = 0.12f;
    g_ball_balance_config.vehicle_turn_compensation_slew_deg_s = 30.0f;
    g_ball_balance_config.minimum_move_error_mm = 8.0f;
    g_ball_balance_config.minimum_move_stationary_delta_mm = 3.0f;
    g_ball_balance_config.minimum_move_detect_ms = 200U;
    g_ball_balance_config.minimum_move_cooldown_ms = 350U;
    g_ball_balance_config.launch_compensation_requires_stationary = true;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.minimum_move_servo_start_deg = 26.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 34.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 8.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 32.0f;
    g_ball_balance_config.negative_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.negative_near_minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_start_deg = 24.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_max_deg = 28.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_ramp_deg_s = 6.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_slew_deg_s = 32.0f;
    g_ball_balance_config.positive_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.positive_near_minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.negative_return_assist_error_max_mm = 20.0f;
    g_ball_balance_config.negative_return_assist_speed_mm_s = 25.0f;
    g_ball_balance_config.negative_return_assist_release_error_mm = 3.0f;
    g_ball_balance_config.negative_return_assist_brake_limit_mm_s2 = 40.0f;
    g_ball_balance_config.vehicle_braking_vision_timeout_ms = 250U;
}

static void BallBalance_ApplyTask5Tune(void)
{
    /* Task 5: lower cruise energy and start braking before the delayed
     * camera/servo chain can carry the ball across the center. */
    BallBalance_ApplyConservativeTune();
    g_ball_balance_config.position_to_velocity_kp_s = 1.45f;
    g_ball_balance_config.max_target_velocity_mm_s = 60.0f;
    g_ball_balance_config.minimum_target_velocity_mm_s = 10.0f;
    g_ball_balance_config.minimum_target_velocity_full_error_mm = 15.0f;
    g_ball_balance_config.motion_prediction_time_s = 0.20f;
    g_ball_balance_config.velocity_limit_overspeed_kp_s = 8.0f;
    g_ball_balance_config.velocity_limit_max_brake_mm_s2 = 340.0f;
    g_ball_balance_config.velocity_reference_accel_limit_mm_s2 = 90.0f;
    g_ball_balance_config.braking_acceleration_mm_s2 = 140.0f;
    g_ball_balance_config.distance_brake_gain = 1.00f;
    g_ball_balance_config.distance_brake_stop_offset_mm = 0.0f;
    g_ball_balance_config.velocity_to_acceleration_kp_s = 6.2f;
    g_ball_balance_config.acceleration_limit_near_mm_s2 = 55.0f;
    g_ball_balance_config.acceleration_limit_far_mm_s2 = 120.0f;
    g_ball_balance_config.acceleration_limit_brake_mm_s2 = 340.0f;
    g_ball_balance_config.acceleration_feedback_gain = 0.04f;
    g_ball_balance_config.servo_accel_slew_deg_s = 60.0f;
    g_ball_balance_config.servo_brake_slew_deg_s = 260.0f;
    g_ball_balance_config.velocity_integral_gain_deg_per_mm = 0.10f;
    g_ball_balance_config.velocity_integral_deadband_mm_s = 0.5f;
    g_ball_balance_config.servo_hold_bias_deg = -2.0f;
    g_ball_balance_config.velocity_integral_limit_deg = 3.5f;
    g_ball_balance_config.servo_normal_angle_limit_deg = 28.0f;
    g_ball_balance_config.servo_near_target_min_limit_deg = 5.0f;
    g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s = 50.0f;
    g_ball_balance_config.settle_position_tolerance_mm = 4.0f;
    g_ball_balance_config.tracked_gain_scale = 0.75f;

    /* The old 2.65 gain amplified a 1 cm/s encoder ripple into roughly a
     * 30 degree feedforward step.  Speeds are now radius-corrected in main.c,
     * so unity gain is the physical longitudinal acceleration estimate. */
    g_ball_balance_config.vehicle_feedforward_gain = 1.05f;
    /* Encoder acceleration leads the command frame at launch; preserve most
     * of the compensation through the first wheel-speed sign reversal. */
    g_ball_balance_config.vehicle_launch_hold_ratio = 0.85f;
    g_ball_balance_config.vehicle_launch_hold_max_ms = 450U;
    g_ball_balance_config.vehicle_command_acceleration_weight = 0.70f;
    g_ball_balance_config.vehicle_command_acceleration_lead_weight = 0.90f;
    g_ball_balance_config.vehicle_measured_acceleration_takeover_ratio = 0.85f;
    g_ball_balance_config.vehicle_measured_acceleration_filter_alpha = 0.30f;
    g_ball_balance_config.vehicle_command_acceleration_filter_alpha = 0.55f;
    g_ball_balance_config.vehicle_acceleration_deadband_mm_s2 = 80.0f;
    g_ball_balance_config.vehicle_acceleration_limit_mm_s2 = 600.0f;
    /* The latest task-5 logs show a 35--40 degree feedforward impulse while
     * the measured chassis speed is still catching up.  Keep enough angle to
     * reject launch acceleration, but leave the cascade room to brake. */
    g_ball_balance_config.vehicle_feedforward_servo_limit_deg = 20.0f;
    g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s = 280.0f;
    g_ball_balance_config.vehicle_feedforward_release_slew_deg_s = 100.0f;
    g_ball_balance_config.vehicle_feedforward_command_reserve_ratio = 0.50f;
    /* Give parking a little extra authority only while the ball is leaving
     * the accepted window. Keeping 32 degrees available throughout the whole
     * deceleration caused a repeatable high-speed reversal after stopping. */
    g_ball_balance_config.vehicle_braking_servo_extra_deg = 4.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_error_mm = 20.0f;
    g_ball_balance_config.vehicle_braking_feedforward_preload_error_mm = 0.0f;
    /* Forward chassis acceleration moves the ball toward negative camera
     * position.  The installed servo direction is already applied later to
     * the combined command, so the feedforward must stay positive internally
     * to produce the same physical servo polarity that returns a negative
     * ball-position error toward zero. */
    g_ball_balance_config.vehicle_feedforward_direction = 1;
    /* Both semicircles in the measured task-5 lap run at about -30 dps and
     * leave the ball near +1 cm.  Apply about two degrees only while turning. */
    g_ball_balance_config.vehicle_turn_compensation_gain_deg_per_dps = 0.10f;
    g_ball_balance_config.vehicle_turn_compensation_deadband_dps = 5.0f;
    g_ball_balance_config.vehicle_turn_compensation_limit_deg = 2.5f;
    g_ball_balance_config.vehicle_turn_compensation_filter_alpha = 0.12f;
    g_ball_balance_config.vehicle_turn_compensation_slew_deg_s = 30.0f;
    g_ball_balance_config.minimum_move_error_mm = 8.0f;
    g_ball_balance_config.minimum_move_stationary_delta_mm = 3.0f;
    g_ball_balance_config.minimum_move_detect_ms = 200U;
    g_ball_balance_config.minimum_move_cooldown_ms = 350U;
    g_ball_balance_config.launch_compensation_requires_stationary = true;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 15.0f;
    /* On the positive side the -2 degree hold bias leaves the old 28 degree
     * breakaway command at only +26 degrees actual output. Static tests show
     * that side needs about +30 degrees before rolling. */
    g_ball_balance_config.minimum_move_servo_start_deg = 26.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 34.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 8.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 32.0f;
    /* Negative beam side has a repeatable ~23 degree breakaway threshold.
     * Use a separate short-range correction to avoid growing to 30+ degrees. */
    g_ball_balance_config.negative_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.negative_near_minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_start_deg = 24.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_max_deg = 28.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_ramp_deg_s = 6.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_slew_deg_s = 32.0f;
    /* The positive side also sticks if the 2 mm displacement release fires
     * before rolling speed is established. Keep breakaway control until the
     * ball reaches 1.5 cm/s toward the target. */
    g_ball_balance_config.positive_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.positive_near_minimum_move_release_speed_mm_s = 15.0f;
    /* Carry enough speed through the negative-side mechanical dead zone,
     * then restore full braking just before the actual target. */
    g_ball_balance_config.negative_return_assist_error_max_mm = 20.0f;
    g_ball_balance_config.negative_return_assist_speed_mm_s = 25.0f;
    g_ball_balance_config.negative_return_assist_release_error_mm = 3.0f;
    g_ball_balance_config.negative_return_assist_brake_limit_mm_s2 = 40.0f;
    /* Keep the last valid camera sample through a short parking UART/frame
     * gap. Normal launch, cruise and curve control retain the 100 ms timeout. */
    g_ball_balance_config.vehicle_braking_vision_timeout_ms = 250U;
}

static void BallBalance_ApplyTask6Tune(void)
{
    /* Independent task-6 moving profile. Keep every task-specific value here. */
    BallBalance_ApplyConservativeTune();
    g_ball_balance_config.position_to_velocity_kp_s = 1.45f;
    g_ball_balance_config.max_target_velocity_mm_s = 60.0f;
    g_ball_balance_config.minimum_target_velocity_mm_s = 10.0f;
    g_ball_balance_config.minimum_target_velocity_full_error_mm = 15.0f;
    g_ball_balance_config.motion_prediction_time_s = 0.20f;
    g_ball_balance_config.velocity_limit_overspeed_kp_s = 8.0f;
    g_ball_balance_config.velocity_limit_max_brake_mm_s2 = 340.0f;
    g_ball_balance_config.velocity_reference_accel_limit_mm_s2 = 90.0f;
    g_ball_balance_config.braking_acceleration_mm_s2 = 140.0f;
    g_ball_balance_config.distance_brake_gain = 1.00f;
    g_ball_balance_config.distance_brake_stop_offset_mm = 0.0f;
    g_ball_balance_config.velocity_to_acceleration_kp_s = 6.2f;
    g_ball_balance_config.acceleration_limit_near_mm_s2 = 55.0f;
    g_ball_balance_config.acceleration_limit_far_mm_s2 = 120.0f;
    g_ball_balance_config.acceleration_limit_brake_mm_s2 = 340.0f;
    g_ball_balance_config.acceleration_feedback_gain = 0.04f;
    g_ball_balance_config.servo_accel_slew_deg_s = 60.0f;
    g_ball_balance_config.servo_brake_slew_deg_s = 260.0f;
    g_ball_balance_config.velocity_integral_gain_deg_per_mm = 0.10f;
    g_ball_balance_config.velocity_integral_deadband_mm_s = 0.5f;
    g_ball_balance_config.servo_hold_bias_deg = -2.0f;
    g_ball_balance_config.velocity_integral_limit_deg = 3.5f;
    g_ball_balance_config.servo_normal_angle_limit_deg = 28.0f;
    g_ball_balance_config.servo_near_target_min_limit_deg = 5.0f;
    g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s = 50.0f;
    g_ball_balance_config.settle_position_tolerance_mm = 4.0f;
    g_ball_balance_config.tracked_gain_scale = 0.75f;
    g_ball_balance_config.vehicle_feedforward_gain = 1.05f;
    g_ball_balance_config.vehicle_launch_hold_ratio = 0.85f;
    g_ball_balance_config.vehicle_launch_hold_max_ms = 450U;
    g_ball_balance_config.vehicle_command_acceleration_weight = 0.70f;
    g_ball_balance_config.vehicle_command_acceleration_lead_weight = 0.90f;
    g_ball_balance_config.vehicle_measured_acceleration_takeover_ratio = 0.85f;
    g_ball_balance_config.vehicle_measured_acceleration_filter_alpha = 0.30f;
    g_ball_balance_config.vehicle_command_acceleration_filter_alpha = 0.55f;
    g_ball_balance_config.vehicle_acceleration_deadband_mm_s2 = 80.0f;
    g_ball_balance_config.vehicle_acceleration_limit_mm_s2 = 600.0f;
    g_ball_balance_config.vehicle_feedforward_servo_limit_deg = 20.0f;
    g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s = 280.0f;
    g_ball_balance_config.vehicle_feedforward_release_slew_deg_s = 100.0f;
    g_ball_balance_config.vehicle_feedforward_command_reserve_ratio = 0.50f;
    g_ball_balance_config.vehicle_braking_servo_extra_deg = 4.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_error_mm = 20.0f;
    g_ball_balance_config.vehicle_braking_feedforward_preload_error_mm = 0.0f;
    g_ball_balance_config.vehicle_feedforward_direction = 1;
    g_ball_balance_config.vehicle_turn_compensation_gain_deg_per_dps = 0.10f;
    g_ball_balance_config.vehicle_turn_compensation_deadband_dps = 5.0f;
    g_ball_balance_config.vehicle_turn_compensation_limit_deg = 2.5f;
    g_ball_balance_config.vehicle_turn_compensation_filter_alpha = 0.12f;
    g_ball_balance_config.vehicle_turn_compensation_slew_deg_s = 30.0f;
    /* Task 6 cruises about 8 mm inside the -7.4 cm target. Camera velocity
     * noise reaches 1--2 cm/s while the ball is mechanically stuck, so keep
     * breakaway active until motion toward the target is unambiguous. */
    g_ball_balance_config.minimum_move_error_mm = 6.0f;
    g_ball_balance_config.minimum_move_stationary_delta_mm = 4.0f;
    g_ball_balance_config.minimum_move_detect_ms = 120U;
    g_ball_balance_config.minimum_move_cooldown_ms = 250U;
    g_ball_balance_config.launch_compensation_requires_stationary = true;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 30.0f;
    g_ball_balance_config.minimum_move_servo_start_deg = 28.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 36.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 12.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 70.0f;
    g_ball_balance_config.negative_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.negative_near_minimum_move_release_speed_mm_s = 15.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_start_deg = 24.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_max_deg = 28.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_ramp_deg_s = 6.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_slew_deg_s = 32.0f;
    g_ball_balance_config.positive_near_minimum_move_error_max_mm = 25.0f;
    g_ball_balance_config.positive_near_minimum_move_release_speed_mm_s = 30.0f;
    g_ball_balance_config.negative_return_assist_error_max_mm = 20.0f;
    g_ball_balance_config.negative_return_assist_speed_mm_s = 25.0f;
    g_ball_balance_config.negative_return_assist_release_error_mm = 3.0f;
    g_ball_balance_config.negative_return_assist_brake_limit_mm_s2 = 40.0f;
    g_ball_balance_config.vehicle_braking_vision_timeout_ms = 250U;
}

static void BallBalance_ApplyTask7Tune(void)
{
    /* Independent task-7 copy of task 6 for isolated experimental tuning. */
    BallBalance_ApplyConservativeTune();
    g_ball_balance_config.position_to_velocity_kp_s = 1.45f;
    g_ball_balance_config.max_target_velocity_mm_s = 60.0f;
    g_ball_balance_config.minimum_target_velocity_mm_s = 10.0f;
    g_ball_balance_config.minimum_target_velocity_full_error_mm = 15.0f;
    g_ball_balance_config.motion_prediction_time_s = 0.20f;
    g_ball_balance_config.velocity_limit_overspeed_kp_s = 8.0f;
    g_ball_balance_config.velocity_limit_max_brake_mm_s2 = 340.0f;
    g_ball_balance_config.velocity_reference_accel_limit_mm_s2 = 90.0f;
    g_ball_balance_config.braking_acceleration_mm_s2 = 140.0f;
    g_ball_balance_config.distance_brake_gain = 1.00f;
    g_ball_balance_config.distance_brake_stop_offset_mm = 0.0f;
    g_ball_balance_config.velocity_to_acceleration_kp_s = 6.2f;
    g_ball_balance_config.acceleration_limit_near_mm_s2 = 55.0f;
    g_ball_balance_config.acceleration_limit_far_mm_s2 = 120.0f;
    g_ball_balance_config.acceleration_limit_brake_mm_s2 = 340.0f;
    g_ball_balance_config.acceleration_feedback_gain = 0.04f;
    g_ball_balance_config.servo_accel_slew_deg_s = 60.0f;
    g_ball_balance_config.servo_brake_slew_deg_s = 260.0f;
    g_ball_balance_config.velocity_integral_gain_deg_per_mm = 0.10f;
    g_ball_balance_config.velocity_integral_deadband_mm_s = 0.5f;
    g_ball_balance_config.servo_hold_bias_deg = -2.0f;
    g_ball_balance_config.velocity_integral_limit_deg = 3.5f;
    g_ball_balance_config.servo_normal_angle_limit_deg = 28.0f;
    g_ball_balance_config.servo_near_target_min_limit_deg = 5.0f;
    g_ball_balance_config.servo_near_target_full_brake_velocity_mm_s = 50.0f;
    g_ball_balance_config.settle_position_tolerance_mm = 4.0f;
    g_ball_balance_config.tracked_gain_scale = 0.75f;
    g_ball_balance_config.vehicle_feedforward_gain = 1.05f;
    g_ball_balance_config.vehicle_launch_hold_ratio = 0.85f;
    g_ball_balance_config.vehicle_launch_hold_max_ms = 450U;
    g_ball_balance_config.vehicle_command_acceleration_weight = 0.70f;
    g_ball_balance_config.vehicle_command_acceleration_lead_weight = 0.90f;
    g_ball_balance_config.vehicle_measured_acceleration_takeover_ratio = 0.85f;
    g_ball_balance_config.vehicle_measured_acceleration_filter_alpha = 0.30f;
    g_ball_balance_config.vehicle_command_acceleration_filter_alpha = 0.55f;
    g_ball_balance_config.vehicle_acceleration_deadband_mm_s2 = 80.0f;
    g_ball_balance_config.vehicle_acceleration_limit_mm_s2 = 600.0f;
    g_ball_balance_config.vehicle_feedforward_servo_limit_deg = 20.0f;
    g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s = 280.0f;
    g_ball_balance_config.vehicle_feedforward_release_slew_deg_s = 100.0f;
    g_ball_balance_config.vehicle_feedforward_command_reserve_ratio = 0.50f;
    g_ball_balance_config.vehicle_braking_servo_extra_deg = 4.0f;
    g_ball_balance_config.vehicle_braking_servo_extra_error_mm = 10.0f;
    g_ball_balance_config.vehicle_braking_feedforward_preload_error_mm = 0.0f;
    g_ball_balance_config.vehicle_feedforward_direction = 1;
    g_ball_balance_config.vehicle_turn_compensation_gain_deg_per_dps = 0.10f;
    g_ball_balance_config.vehicle_turn_compensation_deadband_dps = 5.0f;
    g_ball_balance_config.vehicle_turn_compensation_limit_deg = 2.5f;
    g_ball_balance_config.vehicle_turn_compensation_filter_alpha = 0.12f;
    g_ball_balance_config.vehicle_turn_compensation_slew_deg_s = 30.0f;
    /* Task 7 repeatedly stalls about 1.2 cm inside +7 cm. Its measured
     * velocity noise prematurely released every -30 degree breakaway pulse
     * after only 0.2--0.6 s, before the angle could ramp toward -38 degrees.
     * Accept the noisy position as stationary and release only on clear
     * target-directed motion or entry into the 6 mm breakaway window. */
    g_ball_balance_config.minimum_move_error_mm = 6.0f;
    g_ball_balance_config.minimum_move_stationary_delta_mm = 5.0f;
    g_ball_balance_config.minimum_move_detect_ms = 120U;
    g_ball_balance_config.minimum_move_cooldown_ms = 180U;
    g_ball_balance_config.launch_compensation_requires_stationary = true;
    g_ball_balance_config.minimum_move_release_speed_mm_s = 35.0f;
    g_ball_balance_config.minimum_move_servo_start_deg = 26.0f;
    g_ball_balance_config.minimum_move_servo_max_deg = 34.0f;
    g_ball_balance_config.minimum_move_servo_ramp_deg_s = 12.0f;
    g_ball_balance_config.minimum_move_servo_slew_deg_s = 90.0f;
    g_ball_balance_config.negative_near_minimum_move_error_max_mm = 120.0f;
    g_ball_balance_config.negative_near_minimum_move_release_speed_mm_s = 35.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_start_deg = 30.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_max_deg = 38.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_ramp_deg_s = 16.0f;
    g_ball_balance_config.negative_near_minimum_move_servo_slew_deg_s = 90.0f;
    g_ball_balance_config.positive_near_minimum_move_error_max_mm = 20.0f;
    g_ball_balance_config.positive_near_minimum_move_release_speed_mm_s = 25.0f;
    g_ball_balance_config.negative_return_assist_error_max_mm = 20.0f;
    g_ball_balance_config.negative_return_assist_speed_mm_s = 25.0f;
    g_ball_balance_config.negative_return_assist_release_error_mm = 3.0f;
    g_ball_balance_config.negative_return_assist_brake_limit_mm_s2 = 40.0f;
    g_ball_balance_config.vehicle_braking_vision_timeout_ms = 250U;
}

void BallBalance_Init(void)
{
    memset(&g_maixcam_parser, 0, sizeof(g_maixcam_parser));
    memset((void *)&g_published_sample, 0, sizeof(g_published_sample));
    memset(&g_ball_balance_status, 0, sizeof(g_ball_balance_status));
    BallBalance_ApplyConservativeTune();

    g_publish_revision = 0U;
    g_accepted_packet_count = 0U;
    g_crc_error_count = 0U;
    g_format_error_count = 0U;
    g_sequence_drop_count = 0U;
    g_last_rx_sequence = 0U;
    g_last_sequence_received_ms = 0U;
    g_rx_sequence_initialized = false;
    g_last_control_revision = 0U;
    g_filtered_position_mm = 0.0f;
    g_filtered_velocity_mm_s = 0.0f;
    g_target_reference_mm = 0.0f;
    g_position_gain_scale = 1.0f;
    g_task3_control_profile_active = false;
    g_task3_negative_target_reached = false;
    BallBalance_ResetCascadeState();
    g_vehicle_feedforward_enabled = false;
    BallBalance_ResetVehicleMotion();
    g_ball_balance_config.servo_neutral_angle_deg =
        BallBalance_ClampServoAngle(
            g_ball_balance_config.servo_neutral_angle_deg);
    g_last_servo_angle_deg = g_ball_balance_config.servo_neutral_angle_deg;
    g_filter_initialized = false;
    g_was_vision_usable = false;

    g_ball_balance_status.protocol_self_test_ok =
        Maixcam_ProtocolSelfTest();
}

void BallBalance_UART1_RxByte(uint8_t byte)
{
    MaixcamPacket packet;
    uint32_t now_ms;
    bool packet_complete;

    packet_complete = Maixcam_ParserPush(
        &g_maixcam_parser, byte, &packet);
    g_crc_error_count = g_maixcam_parser.crc_errors;
    g_format_error_count = g_maixcam_parser.format_errors;
    if (!packet_complete) {
        return;
    }

    now_ms = get_systick_ms();
    if (g_rx_sequence_initialized &&
        ((uint32_t)(now_ms - g_last_sequence_received_ms) <=
         MAIXCAM_SEQUENCE_RESET_MS) &&
        !BallBalance_SequenceIsNewer(packet.sequence, g_last_rx_sequence)) {
        g_sequence_drop_count++;
        return;
    }

    g_last_rx_sequence = packet.sequence;
    g_last_sequence_received_ms = now_ms;
    g_rx_sequence_initialized = true;

    g_publish_revision++;
    __DMB();
    g_published_sample.present = 1U;
    g_published_sample.flags = packet.flags;
    g_published_sample.sequence = packet.sequence;
    g_published_sample.timestamp_ms = packet.timestamp_ms;
    g_published_sample.position_mm = packet.position_mm;
    g_published_sample.velocity_mm_s = packet.velocity_mm_s;
    g_published_sample.confidence_milli = packet.confidence_milli;
    g_published_sample.processing_us = packet.processing_us;
    g_published_sample.received_at_ms = now_ms;
    __DMB();
    g_publish_revision++;
    g_accepted_packet_count++;
}

void BallBalance_UART1_ResetParser(void)
{
    g_maixcam_parser.index = 0U;
}

void BallBalance_Control200Hz(void)
{
    PublishedVisionSample sample;
    uint32_t revision = 0U;
    uint32_t now_ms = get_systick_ms();
    uint32_t sample_age_ms = 0U;
    uint32_t vision_timeout_ms;
    bool has_sample;
    bool sample_usable = false;
    bool measured = false;
    bool tracked = false;
    bool new_sample;
    bool controller_active;
    bool settled = false;
    float position_alpha;
    float velocity_alpha;
    float position_error = 0.0f;
    float requested_target_velocity_mm_s = 0.0f;
    float previous_target_velocity_mm_s;
    float target_velocity_delta;
    float maximum_target_velocity_step;
    float maximum_target_deceleration_step;
    float reference_acceleration_mm_s2 = 0.0f;
    float velocity_error_mm_s = 0.0f;
    float target_acceleration_mm_s2 = 0.0f;
    float acceleration_error_mm_s2 = 0.0f;
    float corrected_acceleration_mm_s2 = 0.0f;
    float acceleration_limit_mm_s2;
    float acceleration_limit_near;
    float acceleration_limit_far;
    float acceleration_urgency = 0.0f;
    float acceleration_full_error;
    float maximum_target_velocity;
    float planned_velocity_limit;
    float velocity_limit_start;
    float velocity_limit_range;
    float velocity_limit_urgency;
    float velocity_limit_acceleration_ceiling_mm_s2;
    float velocity_limit_brake_acceleration_mm_s2 = 0.0f;
    float velocity_limited_acceleration_mm_s2;
    float absolute_ball_velocity_mm_s;
    float overspeed_mm_s;
    float overspeed_kp;
    float maximum_velocity_limit_brake;
    float velocity_direction;
    float distance_brake_gain;
    float distance_brake_remaining_mm;
    float distance_brake_available_mm;
    float distance_brake_limit_mm_s2;
    float negative_return_assist_error_max_mm;
    float negative_return_assist_speed_mm_s;
    float negative_return_assist_release_error_mm;
    float negative_return_assist_brake_limit_mm_s2;
    bool cruise_velocity_requested;
    bool velocity_limit_active = false;
    float beam_angle_deg = 0.0f;
    float desired_servo_angle_deg;
    float servo_angle_offset_deg;
    float control_servo_offset_deg;
    float launch_compensation_servo_deg = 0.0f;
    float vehicle_feedforward_servo_deg = 0.0f;
    float vehicle_turn_compensation_servo_deg = 0.0f;
    float vehicle_feedforward_motion_scale = 1.0f;
    float vehicle_feedforward_motion_step;
    float vehicle_feedforward_motion_sign;
    float vehicle_feedforward_reserve_deg = 0.0f;
    float vehicle_feedforward_speed_start_mm_s;
    float vehicle_feedforward_speed_full_mm_s;
    float rack_travel_command_mm = 0.0f;
    float servo_linkage_gain;
    float servo_angle_delta;
    float servo_angle_step;
    float servo_slew_rate_deg_s;
    float servo_normal_angle_limit_deg;
    float servo_effective_angle_limit_deg;
    float servo_vehicle_angle_limit_deg;
    float servo_internal_offset_deg;
    float current_servo_offset_deg;
    float servo_urgency = 0.0f;
    float direction;
    bool servo_neutral_transition = false;
    bool negative_near_minimum_move = false;
    uint16_t servo_speed_command;

    vision_timeout_ms = g_ball_balance_config.vision_timeout_ms;
    if (g_vehicle_braking_active &&
        (g_ball_balance_config.vehicle_braking_vision_timeout_ms >
         vision_timeout_ms)) {
        vision_timeout_ms =
            g_ball_balance_config.vehicle_braking_vision_timeout_ms;
    }
    has_sample = BallBalance_ReadSnapshot(&sample, &revision);
    if (has_sample) {
        sample_age_ms = (uint32_t)(now_ms - sample.received_at_ms);
        measured = (sample.flags & MAIXCAM_FLAG_MEASURED) != 0U;
        tracked = (sample.flags & MAIXCAM_FLAG_TRACKED) != 0U;
        sample_usable =
            ((sample.flags & MAIXCAM_FLAG_VALID) != 0U) &&
            (sample_age_ms <= vision_timeout_ms) &&
            (sample.confidence_milli >=
             g_ball_balance_config.minimum_confidence_milli) &&
            (sample.confidence_milli <= 1000U) &&
            ((int32_t)sample.position_mm >=
             -BALL_POSITION_PHYSICAL_LIMIT_MM) &&
            ((int32_t)sample.position_mm <=
             BALL_POSITION_PHYSICAL_LIMIT_MM) &&
            ((int32_t)sample.velocity_mm_s >=
             -BALL_VELOCITY_PHYSICAL_LIMIT_MM_S) &&
            ((int32_t)sample.velocity_mm_s <=
             BALL_VELOCITY_PHYSICAL_LIMIT_MM_S);
    } else {
        memset(&sample, 0, sizeof(sample));
    }

    new_sample = revision != g_last_control_revision;
    if ((new_sample || !g_filter_initialized) &&
        sample_usable) {
        position_alpha = BallBalance_ClampFloat(
            g_ball_balance_config.position_filter_alpha, 0.0f, 1.0f);
        velocity_alpha = BallBalance_ClampFloat(
            g_ball_balance_config.velocity_filter_alpha, 0.0f, 1.0f);

        if (!g_filter_initialized) {
            g_filtered_position_mm = (float)sample.position_mm;
            g_filtered_velocity_mm_s = (float)sample.velocity_mm_s;
            g_filter_initialized = true;
        } else {
            g_filtered_position_mm += position_alpha *
                ((float)sample.position_mm - g_filtered_position_mm);
            g_filtered_velocity_mm_s += velocity_alpha *
                ((float)sample.velocity_mm_s - g_filtered_velocity_mm_s);
        }
    }
    g_last_control_revision = revision;

    BallBalance_UpdateAccelerationEstimate(
        &sample, new_sample && sample_usable, measured);
    BallBalance_UpdateTargetReference();
    direction = (g_ball_balance_servo_direction < 0) ? -1.0f : 1.0f;
    if (BallBalance_AbsFloat(g_ball_balance_config.servo_gear_radius_mm) >
        0.001f) {
        servo_linkage_gain = BallBalance_AbsFloat(
            g_ball_balance_config.beam_length_mm) /
            BallBalance_AbsFloat(
                g_ball_balance_config.servo_gear_radius_mm);
    } else {
        servo_linkage_gain = 0.0f;
    }
    acceleration_limit_near = BallBalance_AbsFloat(
        g_ball_balance_config.acceleration_limit_near_mm_s2);
    acceleration_limit_far = BallBalance_AbsFloat(
        g_ball_balance_config.acceleration_limit_far_mm_s2);
    if (acceleration_limit_far < acceleration_limit_near) {
        float swap = acceleration_limit_near;
        acceleration_limit_near = acceleration_limit_far;
        acceleration_limit_far = swap;
    }
    acceleration_limit_mm_s2 = acceleration_limit_near;
    servo_speed_command = g_ball_balance_config.servo_speed;
    if (servo_speed_command == 0U) {
        servo_speed_command = 1U;
    }
    controller_active = g_ball_balance_enabled && sample_usable;
    if (controller_active) {
        position_error = g_target_reference_mm - g_filtered_position_mm;
        negative_return_assist_error_max_mm = BallBalance_AbsFloat(
            g_ball_balance_config.negative_return_assist_error_max_mm);
        negative_return_assist_speed_mm_s = BallBalance_AbsFloat(
            g_ball_balance_config.negative_return_assist_speed_mm_s);
        negative_return_assist_release_error_mm = BallBalance_AbsFloat(
            g_ball_balance_config.negative_return_assist_release_error_mm);
        negative_return_assist_brake_limit_mm_s2 = BallBalance_AbsFloat(
            g_ball_balance_config.negative_return_assist_brake_limit_mm_s2);

        if (g_negative_return_assist_active) {
            if ((position_error <= negative_return_assist_release_error_mm) ||
                (position_error > negative_return_assist_error_max_mm) ||
                (negative_return_assist_speed_mm_s <= 0.001f)) {
                g_negative_return_assist_active = false;
                BallBalance_ResetMotionProfile();
            }
        } else if ((negative_return_assist_error_max_mm > 0.001f) &&
                   (negative_return_assist_speed_mm_s > 0.001f) &&
                   (position_error > BallBalance_AbsFloat(
                        g_ball_balance_config.settle_position_tolerance_mm)) &&
                   (position_error <= negative_return_assist_error_max_mm)) {
            g_negative_return_assist_active = true;
            BallBalance_ResetMotionProfile();
        }
        negative_near_minimum_move =
            (position_error > 0.0f) &&
            (g_ball_balance_config.
                negative_near_minimum_move_error_max_mm > 0.001f) &&
            (position_error <= g_ball_balance_config.
                negative_near_minimum_move_error_max_mm);
        settled = measured && !g_negative_return_assist_active &&
            (BallBalance_AbsFloat(position_error) <=
             BallBalance_AbsFloat(
                 g_ball_balance_config.settle_position_tolerance_mm)) &&
            (BallBalance_AbsFloat(g_filtered_velocity_mm_s) <=
             BallBalance_AbsFloat(
                 g_ball_balance_config.settle_velocity_tolerance_mm_s));

        if (g_negative_return_assist_active) {
            requested_target_velocity_mm_s =
                negative_return_assist_speed_mm_s;
            g_motion_direction = 1.0f;
            g_profile_peak_velocity_mm_s =
                negative_return_assist_speed_mm_s;
            g_profile_braking_velocity_mm_s =
                negative_return_assist_speed_mm_s;
            g_motion_profile_initialized = true;
            g_motion_phase = BALL_BALANCE_MOTION_CRUISE;
        } else {
            requested_target_velocity_mm_s =
                BallBalance_CalculateProfileVelocity(
                    position_error, g_filtered_velocity_mm_s);
        }
        previous_target_velocity_mm_s = g_target_velocity_mm_s;
        maximum_target_velocity_step = BallBalance_AbsFloat(
            g_ball_balance_config.velocity_reference_accel_limit_mm_s2) *
            BALL_CONTROL_PERIOD_S;
        maximum_target_deceleration_step = BallBalance_AbsFloat(
            g_ball_balance_config.braking_acceleration_mm_s2) *
            BALL_CONTROL_PERIOD_S;
        if ((g_position_gain_scale > 1.05f) &&
            (requested_target_velocity_mm_s > 0.0f)) {
            /* This is a short pass-through leg. Apply the cruise reference
             * immediately so the normal reference ramp cannot leave the ball
             * below static-friction speed for several hundred milliseconds. */
            g_target_velocity_mm_s = requested_target_velocity_mm_s;
        } else if (g_task3_control_profile_active &&
                   (g_target_reference_mm < -0.001f) &&
                   (BallBalance_AbsFloat(position_error) <=
                    BallBalance_AbsFloat(
                        g_ball_balance_config.
                            task3_negative_minimum_move_error_mm))) {
            /* Crossing into the accepted negative window is the braking
             * event. Drop the reference immediately; the velocity loop then
             * removes residual route speed before the stable timer expires. */
            g_target_velocity_mm_s = 0.0f;
        } else if (!g_negative_return_assist_active &&
            (BallBalance_AbsFloat(position_error) <=
            BallBalance_AbsFloat(
                g_ball_balance_config.settle_position_tolerance_mm))) {
            g_target_velocity_mm_s = 0.0f;
        } else if ((requested_target_velocity_mm_s *
                    g_target_velocity_mm_s) < 0.0f) {
            /* Never carry a velocity reference through the target. */
            g_target_velocity_mm_s = 0.0f;
        } else if (BallBalance_AbsFloat(requested_target_velocity_mm_s) <
                   BallBalance_AbsFloat(g_target_velocity_mm_s)) {
            target_velocity_delta = requested_target_velocity_mm_s -
                                    g_target_velocity_mm_s;
            target_velocity_delta = BallBalance_ClampFloat(
                target_velocity_delta,
                -maximum_target_deceleration_step,
                maximum_target_deceleration_step);
            g_target_velocity_mm_s += target_velocity_delta;
        } else {
            target_velocity_delta = requested_target_velocity_mm_s -
                                    g_target_velocity_mm_s;
            target_velocity_delta = BallBalance_ClampFloat(
                target_velocity_delta,
                -maximum_target_velocity_step,
                maximum_target_velocity_step);
            g_target_velocity_mm_s += target_velocity_delta;
        }
        reference_acceleration_mm_s2 =
            (g_target_velocity_mm_s - previous_target_velocity_mm_s) /
            BALL_CONTROL_PERIOD_S;
        velocity_error_mm_s = g_target_velocity_mm_s -
                              g_filtered_velocity_mm_s;

        acceleration_full_error = BallBalance_AbsFloat(
            g_ball_balance_config.acceleration_limit_full_error_mm);
        maximum_target_velocity =
            BallBalance_GetActiveMaximumTargetVelocity();
        if ((g_motion_phase == BALL_BALANCE_MOTION_DECELERATE) ||
            (g_motion_phase == BALL_BALANCE_MOTION_HOLD)) {
            planned_velocity_limit = BallBalance_AbsFloat(
                requested_target_velocity_mm_s);
        } else {
            planned_velocity_limit = BallBalance_MaxFloat(
                BallBalance_AbsFloat(g_profile_peak_velocity_mm_s),
                BallBalance_AbsFloat(requested_target_velocity_mm_s));
        }
        planned_velocity_limit = BallBalance_MaxFloat(
            planned_velocity_limit,
            BallBalance_AbsFloat(
                g_ball_balance_config.settle_velocity_tolerance_mm_s));
        maximum_target_velocity = BallBalance_MinFloat(
            maximum_target_velocity, planned_velocity_limit);
        if (acceleration_full_error > 0.001f) {
            acceleration_urgency = BallBalance_MaxFloat(
                acceleration_urgency,
                BallBalance_AbsFloat(position_error) /
                acceleration_full_error);
        }
        if (maximum_target_velocity > 0.001f) {
            acceleration_urgency = BallBalance_MaxFloat(
                acceleration_urgency,
                BallBalance_AbsFloat(g_filtered_velocity_mm_s) /
                maximum_target_velocity);
            acceleration_urgency = BallBalance_MaxFloat(
                acceleration_urgency,
                BallBalance_AbsFloat(velocity_error_mm_s) /
                maximum_target_velocity);
        }
        acceleration_urgency = BallBalance_Smoothstep01(
            acceleration_urgency);
        acceleration_limit_mm_s2 = acceleration_limit_near +
            (acceleration_limit_far - acceleration_limit_near) *
            acceleration_urgency;

        target_acceleration_mm_s2 =
            g_ball_balance_config.velocity_to_acceleration_kp_s *
                velocity_error_mm_s;
        if ((reference_acceleration_mm_s2 * velocity_error_mm_s) > 0.0f) {
            target_acceleration_mm_s2 +=
                g_ball_balance_config.acceleration_feedforward_gain *
                reference_acceleration_mm_s2;
        }
        if ((target_acceleration_mm_s2 * g_filtered_velocity_mm_s) < 0.0f) {
            acceleration_limit_mm_s2 = BallBalance_MaxFloat(
                acceleration_limit_mm_s2,
                BallBalance_AbsFloat(
                    g_ball_balance_config.acceleration_limit_brake_mm_s2));
        }
        target_acceleration_mm_s2 = BallBalance_ClampFloat(
            target_acceleration_mm_s2,
            -acceleration_limit_mm_s2,
            acceleration_limit_mm_s2);
        if (tracked && !measured) {
            target_acceleration_mm_s2 *= BallBalance_ClampFloat(
                g_ball_balance_config.tracked_gain_scale, 0.0f, 1.0f);
        }

        target_acceleration_mm_s2 = BallBalance_UpdateMinimumMove(
            &sample, new_sample, measured, position_error,
            target_acceleration_mm_s2, now_ms);

        /* Below the measured-speed limit, only remove forward drive. Reverse
         * braking starts after actual overspeed; otherwise the limiter can
         * stop the ball before the target and create a back-and-forth cycle. */
        if (measured &&
            (maximum_target_velocity > 0.001f)) {
            velocity_limit_start = maximum_target_velocity *
                BallBalance_ClampFloat(
                    g_ball_balance_config.velocity_limit_brake_start_ratio,
                    0.0f, 0.95f);
            velocity_limit_range = maximum_target_velocity -
                                   velocity_limit_start;
            absolute_ball_velocity_mm_s =
                BallBalance_AbsFloat(g_filtered_velocity_mm_s);
            if (absolute_ball_velocity_mm_s > velocity_limit_start) {
                velocity_limit_active = true;
                velocity_direction =
                    (g_filtered_velocity_mm_s >= 0.0f) ? 1.0f : -1.0f;
                if (absolute_ball_velocity_mm_s <=
                    maximum_target_velocity) {
                    velocity_limit_urgency = BallBalance_Smoothstep01(
                        (absolute_ball_velocity_mm_s -
                         velocity_limit_start) / velocity_limit_range);
                    velocity_limit_acceleration_ceiling_mm_s2 =
                        acceleration_limit_far *
                        (1.0f - velocity_limit_urgency);
                    if ((target_acceleration_mm_s2 * velocity_direction) >
                        velocity_limit_acceleration_ceiling_mm_s2) {
                        target_acceleration_mm_s2 = velocity_direction *
                            velocity_limit_acceleration_ceiling_mm_s2;
                    }
                } else {
                    overspeed_mm_s = absolute_ball_velocity_mm_s -
                                     maximum_target_velocity;
                    overspeed_kp = BallBalance_AbsFloat(
                        g_ball_balance_config.velocity_limit_overspeed_kp_s);
                    maximum_velocity_limit_brake = BallBalance_AbsFloat(
                        g_ball_balance_config.velocity_limit_max_brake_mm_s2);
                    velocity_limit_brake_acceleration_mm_s2 =
                        BallBalance_ClampFloat(
                            overspeed_kp * overspeed_mm_s,
                            0.0f, maximum_velocity_limit_brake);
                    velocity_limited_acceleration_mm_s2 =
                        -velocity_direction *
                        velocity_limit_brake_acceleration_mm_s2;
                    cruise_velocity_requested =
                        ((requested_target_velocity_mm_s *
                          velocity_direction) >=
                         (0.90f * maximum_target_velocity));
                    if (cruise_velocity_requested ||
                        ((target_acceleration_mm_s2 * velocity_direction) >
                         -velocity_limit_brake_acceleration_mm_s2)) {
                        target_acceleration_mm_s2 =
                            velocity_limited_acceleration_mm_s2;
                    }
                }
            }
        }

        /* When the ball is already travelling toward the target, cap reverse
         * acceleration to the value required to stop at the edge of the
         * accepted position window.  Moving away from or across the target
         * still receives the full emergency brake. */
        distance_brake_gain = BallBalance_AbsFloat(
            g_ball_balance_config.distance_brake_gain);
        if (measured &&
            (distance_brake_gain > 0.001f) &&
            ((position_error * g_filtered_velocity_mm_s) > 0.0f) &&
            ((target_acceleration_mm_s2 *
              g_filtered_velocity_mm_s) < 0.0f)) {
            distance_brake_remaining_mm = BallBalance_MaxFloat(
                BallBalance_AbsFloat(position_error) -
                    BallBalance_AbsFloat(
                        g_ball_balance_config.distance_brake_stop_offset_mm),
                0.0f);
            distance_brake_available_mm = distance_brake_remaining_mm -
                BallBalance_AbsFloat(g_filtered_velocity_mm_s) *
                    BallBalance_ClampFloat(
                        g_ball_balance_config.motion_prediction_time_s,
                        0.0f, 1.0f);
            distance_brake_available_mm = BallBalance_MaxFloat(
                distance_brake_available_mm, 0.5f);
            distance_brake_limit_mm_s2 = distance_brake_gain *
                g_filtered_velocity_mm_s * g_filtered_velocity_mm_s /
                (2.0f * distance_brake_available_mm);
            distance_brake_limit_mm_s2 = BallBalance_MinFloat(
                distance_brake_limit_mm_s2,
                BallBalance_AbsFloat(
                    g_ball_balance_config.acceleration_limit_brake_mm_s2));
            if (BallBalance_AbsFloat(target_acceleration_mm_s2) >
                distance_brake_limit_mm_s2) {
                target_acceleration_mm_s2 =
                    ((g_filtered_velocity_mm_s >= 0.0f) ? -1.0f : 1.0f) *
                    distance_brake_limit_mm_s2;
            }
        }
        /* Keep the negative-side return moving through its dead zone. This
         * only limits braking before the final approach; wrong-way and
         * high-speed emergency braking stay intact. */
        if (g_negative_return_assist_active &&
            (position_error > 0.0f) &&
            (g_filtered_velocity_mm_s > 0.0f) &&
            (target_acceleration_mm_s2 < 0.0f) &&
            (negative_return_assist_brake_limit_mm_s2 > 0.001f) &&
            (g_filtered_velocity_mm_s <= BallBalance_AbsFloat(
                g_ball_balance_config.max_target_velocity_mm_s)) &&
            (target_acceleration_mm_s2 <
             -negative_return_assist_brake_limit_mm_s2)) {
            target_acceleration_mm_s2 =
                -negative_return_assist_brake_limit_mm_s2;
        }
        acceleration_error_mm_s2 = target_acceleration_mm_s2 -
                                    g_filtered_acceleration_mm_s2;
        corrected_acceleration_mm_s2 = target_acceleration_mm_s2 +
            BallBalance_ClampFloat(
                g_ball_balance_config.acceleration_feedback_gain,
                0.0f, 1.0f) * acceleration_error_mm_s2;
        beam_angle_deg = BallBalance_AccelerationToBeamAngle(
            corrected_acceleration_mm_s2);
    } else {
        if (g_was_vision_usable && has_sample &&
            (sample_age_ms > g_ball_balance_config.vision_timeout_ms)) {
            g_ball_balance_status.timeout_count++;
        }
        BallBalance_ResetCascadeState();
        g_filter_initialized = false;
    }
    g_was_vision_usable = sample_usable;
    if (controller_active) {
        launch_compensation_servo_deg =
            BallBalance_CalculateLaunchCompensation(
                position_error, g_target_velocity_mm_s,
                g_filtered_velocity_mm_s);
    }
    BallBalance_UpdateVelocityIntegral(
        controller_active, measured, velocity_limit_active,
        velocity_error_mm_s, corrected_acceleration_mm_s2,
        g_filtered_velocity_mm_s, launch_compensation_servo_deg);

    /* Velocity PI runs continuously, including inside the position window.
     * Its integral retains the angle needed to reject tube slope and vehicle
     * acceleration; measured velocity unwinds that angle during braking. */
    control_servo_offset_deg = corrected_acceleration_mm_s2 *
        BallBalance_AbsFloat(
            g_ball_balance_config.servo_degrees_per_acceleration_mm_s2) +
        g_velocity_integral_servo_deg + launch_compensation_servo_deg;
    if (tracked && !measured) {
        control_servo_offset_deg *= BallBalance_ClampFloat(
            g_ball_balance_config.tracked_gain_scale, 0.0f, 1.0f);
    }
    servo_normal_angle_limit_deg = BallBalance_AbsFloat(
        g_ball_balance_config.servo_normal_angle_limit_deg);
    if (g_target_hold_tune_enabled && g_vehicle_feedforward_enabled &&
        g_vehicle_launch_hold_active) {
        /* At the +7.2 cm endpoint the measured launch inertia still moved the
         * ball about 4 cm backward while the combined command was clipped at
         * -28 degrees. Temporarily use the available mechanical travel until
         * wheel speed settles; the curve limit below always has priority. */
        servo_normal_angle_limit_deg = BallBalance_MaxFloat(
            servo_normal_angle_limit_deg,
            BALL_TARGET_HOLD_LAUNCH_SERVO_LIMIT_DEG);
    }
    if (g_target_hold_tune_enabled && g_vehicle_feedforward_enabled &&
        (BallBalance_AbsFloat(g_vehicle_filtered_yaw_rate_dps) >=
         BALL_TARGET_HOLD_TURN_YAW_RATE_DPS)) {
        servo_normal_angle_limit_deg = BallBalance_MinFloat(
            servo_normal_angle_limit_deg,
            BALL_TARGET_HOLD_TURN_SERVO_LIMIT_DEG);
    }
    if (controller_active && g_vehicle_feedforward_enabled &&
        g_vehicle_braking_active &&
        (BallBalance_AbsFloat(position_error) <= BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_braking_servo_extra_error_mm)) &&
        (BallBalance_AbsFloat(g_filtered_velocity_mm_s) >= 15.0f) &&
        ((position_error * g_filtered_velocity_mm_s) < 0.0f)) {
        servo_normal_angle_limit_deg += BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_braking_servo_extra_deg);
    }
    servo_normal_angle_limit_deg = BallBalance_MinFloat(
        servo_normal_angle_limit_deg,
        BALL_SERVO_MAX_ANGLE_DEG);
    servo_effective_angle_limit_deg = BallBalance_CalculateServoAngleLimit(
        servo_normal_angle_limit_deg,
        position_error,
        g_filtered_velocity_mm_s,
        control_servo_offset_deg);
    control_servo_offset_deg = BallBalance_ClampFloat(
        control_servo_offset_deg,
        -servo_effective_angle_limit_deg,
        servo_effective_angle_limit_deg);
    if (controller_active && g_vehicle_feedforward_enabled) {
        vehicle_feedforward_servo_deg = g_vehicle_feedforward_servo_deg;
        vehicle_turn_compensation_servo_deg =
            g_vehicle_turn_compensation_servo_deg;

        /* Feedforward rejects chassis acceleration, but it must not overpower
         * the cascade after the ball has already acquired speed in the same
         * direction. Fade it from 25% to 75% of the moving-profile ball-speed
         * limit, and remove it if it is pushing an out-of-window ball farther
         * away from the target. */
        vehicle_feedforward_speed_start_mm_s = 0.25f *
            BallBalance_GetActiveMaximumTargetVelocity();
        vehicle_feedforward_speed_full_mm_s = 0.75f *
            BallBalance_GetActiveMaximumTargetVelocity();
        /* Internal feedforward and the cascade use the same ball-acceleration
         * polarity.  Installation direction is applied after they are added;
         * including it here removes the correction while the chassis is still
         * accelerating or braking. */
        if ((vehicle_feedforward_servo_deg *
             g_filtered_velocity_mm_s) > 0.0f &&
            (vehicle_feedforward_speed_full_mm_s >
             vehicle_feedforward_speed_start_mm_s + 0.001f)) {
            vehicle_feedforward_motion_scale = 1.0f -
                BallBalance_Smoothstep01(
                    (BallBalance_AbsFloat(g_filtered_velocity_mm_s) -
                     vehicle_feedforward_speed_start_mm_s) /
                    (vehicle_feedforward_speed_full_mm_s -
                     vehicle_feedforward_speed_start_mm_s));
        }
        if ((BallBalance_AbsFloat(position_error) >
             BallBalance_MaxFloat(
                 BallBalance_AbsFloat(
                     g_ball_balance_config.settle_position_tolerance_mm),
                 g_vehicle_braking_active ?
                     BallBalance_AbsFloat(
                         g_ball_balance_config.
                             vehicle_braking_feedforward_preload_error_mm) :
                     0.0f)) &&
            ((vehicle_feedforward_servo_deg * position_error) < 0.0f) &&
            !(g_vehicle_braking_active &&
              (BallBalance_AbsFloat(
                   g_ball_balance_config.
                       vehicle_braking_feedforward_preload_error_mm) >
               0.001f) &&
              ((position_error * g_filtered_velocity_mm_s) > 0.0f) &&
              ((vehicle_feedforward_servo_deg *
                g_filtered_velocity_mm_s) < 0.0f)) &&
            (!g_target_hold_tune_enabled ||
             ((vehicle_feedforward_servo_deg *
               g_filtered_velocity_mm_s) >= 0.0f))) {
            /* A braking feedforward may point away from the current position
             * error while it opposes a ball already moving toward the target.
             * Task 4 uses a nonzero preload window to keep that valid braking
             * action; task 7 retains its target-hold exception below. */
            vehicle_feedforward_motion_scale = 0.0f;
        }
        vehicle_feedforward_motion_scale = BallBalance_ClampFloat(
            vehicle_feedforward_motion_scale, 0.0f, 1.0f);
        if (BallBalance_AbsFloat(g_vehicle_feedforward_servo_deg) > 0.05f) {
            vehicle_feedforward_motion_sign =
                (g_vehicle_feedforward_servo_deg < 0.0f) ? -1.0f : 1.0f;
            if ((g_vehicle_feedforward_motion_sign != 0.0f) &&
                (vehicle_feedforward_motion_sign !=
                 g_vehicle_feedforward_motion_sign)) {
                g_vehicle_feedforward_motion_scale = 1.0f;
            }
            g_vehicle_feedforward_motion_sign =
                vehicle_feedforward_motion_sign;
        }
        if (vehicle_feedforward_motion_scale <
            g_vehicle_feedforward_motion_scale) {
            vehicle_feedforward_motion_step =
                BallBalance_AbsFloat(
                    g_ball_balance_config.vehicle_feedforward_release_slew_deg_s) /
                BallBalance_MaxFloat(
                    BallBalance_AbsFloat(
                        g_ball_balance_config.vehicle_feedforward_servo_limit_deg),
                    1.0f) * BALL_CONTROL_PERIOD_S;
        } else {
            vehicle_feedforward_motion_step =
                BallBalance_AbsFloat(
                    (g_ball_balance_config.vehicle_launch_preload_enabled &&
                     g_vehicle_launch_hold_active &&
                     (BallBalance_AbsFloat(g_ball_balance_config.
                        vehicle_launch_feedforward_slew_deg_s) > 0.001f)) ?
                        g_ball_balance_config.
                            vehicle_launch_feedforward_slew_deg_s :
                        g_ball_balance_config.
                            vehicle_feedforward_servo_slew_deg_s) /
                BallBalance_MaxFloat(
                    BallBalance_AbsFloat(
                        g_ball_balance_config.vehicle_feedforward_servo_limit_deg),
                    1.0f) * BALL_CONTROL_PERIOD_S;
        }
        g_vehicle_feedforward_motion_scale += BallBalance_ClampFloat(
            vehicle_feedforward_motion_scale -
                g_vehicle_feedforward_motion_scale,
            -vehicle_feedforward_motion_step,
            vehicle_feedforward_motion_step);
        vehicle_feedforward_servo_deg *=
            BallBalance_ClampFloat(
                g_vehicle_feedforward_motion_scale, 0.0f, 1.0f);
    }
    /* Near the target, keep the total moving-task command inside the same
     * distance/velocity envelope as the cascade.  Previously feedforward was
     * added after this limit and could reintroduce a 30--40 degree impulse
     * even when the position loop had already requested a small correction. */
    servo_internal_offset_deg =
        control_servo_offset_deg + vehicle_feedforward_servo_deg +
        vehicle_turn_compensation_servo_deg;
    if (controller_active && g_vehicle_feedforward_enabled &&
        (BallBalance_AbsFloat(g_vehicle_command_acceleration_mm_s2) >
         BallBalance_AbsFloat(
             g_ball_balance_config.vehicle_acceleration_deadband_mm_s2)) &&
        (BallBalance_AbsFloat(vehicle_feedforward_servo_deg) > 0.05f)) {
        /* Reserve only the feedforward that survived the ball position/speed
         * safety fade.  Reserving the raw value kept braking tilt after the
         * ball crossed the center and caused the variable negative overshoot
         * seen in the latest task-5 logs. */
        vehicle_feedforward_reserve_deg =
            vehicle_feedforward_servo_deg * BallBalance_ClampFloat(
                g_ball_balance_config.
                    vehicle_feedforward_command_reserve_ratio,
                0.0f, 1.0f);
        if (((vehicle_feedforward_reserve_deg > 0.0f) &&
             (servo_internal_offset_deg < vehicle_feedforward_reserve_deg)) ||
            ((vehicle_feedforward_reserve_deg < 0.0f) &&
             (servo_internal_offset_deg > vehicle_feedforward_reserve_deg))) {
            servo_internal_offset_deg = vehicle_feedforward_reserve_deg;
        }
    }
    servo_angle_offset_deg = servo_internal_offset_deg * direction;
    if (controller_active) {
        /* Moving tasks need a small fixed angle to reject the measured beam
         * slope. Apply it in actual servo-output polarity after installation
         * direction; stationary task 3 keeps zero bias. */
        servo_angle_offset_deg +=
            g_ball_balance_config.servo_hold_bias_deg;
        servo_angle_offset_deg += g_target_hold_bias_deg;
    }
    if (controller_active && g_vehicle_feedforward_enabled) {
        /* Reserve a bounded amount of angle for measured chassis acceleration.
         * Clamping the total command to the position-only envelope reduced a
         * 22 degree braking feedforward to about 5 degrees near the target;
         * removing the envelope completely allowed the command to hit the
         * mechanical 40 degree stop. */
        servo_vehicle_angle_limit_deg = BallBalance_MinFloat(
            servo_normal_angle_limit_deg,
            servo_effective_angle_limit_deg +
                BallBalance_MaxFloat(
                    BallBalance_AbsFloat(vehicle_feedforward_servo_deg),
                    BallBalance_AbsFloat(vehicle_feedforward_reserve_deg)));
        servo_angle_offset_deg = BallBalance_ClampFloat(
            servo_angle_offset_deg,
            -servo_vehicle_angle_limit_deg,
            servo_vehicle_angle_limit_deg);
    }
    if (controller_active && g_target_hold_tune_enabled &&
        g_vehicle_feedforward_enabled && g_vehicle_launch_hold_active &&
        (g_vehicle_command_acceleration_mm_s2 >
         BallBalance_AbsFloat(
             g_ball_balance_config.vehicle_acceleration_deadband_mm_s2)) &&
        (g_filtered_velocity_mm_s >=
         BALL_TARGET_HOLD_LAUNCH_BRAKE_SPEED_MM_S) &&
        (BallBalance_AbsFloat(position_error) <=
         BALL_TARGET_HOLD_LAUNCH_BRAKE_WINDOW_MM)) {
        /* Both latest launches first recovered to about +8.1 cm, then a
         * +16 degree reverse command stopped the ball into negative velocity
         * while the chassis was still accelerating. Keep a smaller braking
         * tilt until the launch hold ends; full negative recovery remains
         * available as soon as the ball actually reverses. */
        servo_angle_offset_deg = BallBalance_MinFloat(
            servo_angle_offset_deg,
            BALL_TARGET_HOLD_LAUNCH_BRAKE_LIMIT_DEG);
    }
    desired_servo_angle_deg =
        g_ball_balance_config.servo_neutral_angle_deg +
        servo_angle_offset_deg;
    desired_servo_angle_deg = BallBalance_ClampServoAngle(
        desired_servo_angle_deg);
    servo_angle_delta = desired_servo_angle_deg -
                        g_last_servo_angle_deg;
    current_servo_offset_deg = g_last_servo_angle_deg -
        g_ball_balance_config.servo_neutral_angle_deg;
    if (((current_servo_offset_deg * servo_angle_offset_deg) < 0.0f) &&
        (BallBalance_AbsFloat(current_servo_offset_deg) > 0.05f)) {
        desired_servo_angle_deg =
            g_ball_balance_config.servo_neutral_angle_deg;
        servo_angle_delta = desired_servo_angle_deg -
                            g_last_servo_angle_deg;
        servo_neutral_transition = true;
    }
    if (!controller_active || servo_neutral_transition ||
        (((current_servo_offset_deg * servo_angle_offset_deg) >= 0.0f) &&
         (BallBalance_AbsFloat(servo_angle_offset_deg) <
          BallBalance_AbsFloat(current_servo_offset_deg)))) {
        servo_slew_rate_deg_s = BallBalance_AbsFloat(
            g_ball_balance_config.servo_level_slew_deg_s);
    } else if (((current_servo_offset_deg * servo_angle_offset_deg) < 0.0f) ||
               ((corrected_acceleration_mm_s2 *
                 g_filtered_velocity_mm_s) < 0.0f)) {
        servo_slew_rate_deg_s = BallBalance_AbsFloat(
            g_ball_balance_config.servo_brake_slew_deg_s);
    } else {
        servo_slew_rate_deg_s = BallBalance_AbsFloat(
            g_ball_balance_config.servo_accel_slew_deg_s);
    }
    if (controller_active &&
        (BallBalance_AbsFloat(vehicle_feedforward_servo_deg) > 0.05f)) {
        servo_slew_rate_deg_s = BallBalance_MaxFloat(
            servo_slew_rate_deg_s,
            BallBalance_AbsFloat(
                g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s));
    }
    if (g_minimum_move_active) {
        /* A breakaway correction may need a large final angle, but it must
         * build slowly so the ball does not receive a full impulse while the
         * camera still reports it as stationary. */
        servo_slew_rate_deg_s = BallBalance_MinFloat(
            servo_slew_rate_deg_s,
            BallBalance_AbsFloat(
                g_ball_balance_config.minimum_move_servo_slew_deg_s));
        if (negative_near_minimum_move &&
            (g_ball_balance_config.
                negative_near_minimum_move_servo_slew_deg_s > 0.001f)) {
            servo_slew_rate_deg_s = BallBalance_MinFloat(
                servo_slew_rate_deg_s,
                BallBalance_AbsFloat(
                    g_ball_balance_config.
                        negative_near_minimum_move_servo_slew_deg_s));
        }
    }
    servo_angle_step = servo_slew_rate_deg_s * BALL_CONTROL_PERIOD_S;
    servo_angle_delta = BallBalance_ClampFloat(
        servo_angle_delta, -servo_angle_step, servo_angle_step);
    desired_servo_angle_deg = g_last_servo_angle_deg +
                              servo_angle_delta;
    rack_travel_command_mm = BallBalance_ServoOffsetToRackTravel(
        desired_servo_angle_deg -
        g_ball_balance_config.servo_neutral_angle_deg);
    if (controller_active) {
        servo_urgency = BallBalance_CalculateUrgency(
            position_error,
            g_filtered_velocity_mm_s,
            velocity_error_mm_s,
            servo_angle_delta);
        if (acceleration_limit_far > 0.001f) {
            servo_urgency = BallBalance_MaxFloat(
                servo_urgency,
                BallBalance_AbsFloat(target_acceleration_mm_s2) /
                acceleration_limit_far);
        }
        if (BallBalance_AbsFloat(
                g_ball_balance_config.vehicle_feedforward_servo_limit_deg) >
            0.001f) {
            servo_urgency = BallBalance_MaxFloat(
                servo_urgency,
                BallBalance_AbsFloat(vehicle_feedforward_servo_deg) /
                BallBalance_AbsFloat(
                    g_ball_balance_config.vehicle_feedforward_servo_limit_deg));
        }
        servo_urgency = BallBalance_ClampFloat(servo_urgency, 0.0f, 1.0f);
        servo_speed_command = BallBalance_CalculateServoSpeed(
            servo_urgency);
    }
    g_last_servo_angle_deg = desired_servo_angle_deg;
    Servo_Y_SetAngle(g_last_servo_angle_deg, servo_speed_command);

    g_ball_balance_status.vision_valid = sample_usable;
    g_ball_balance_status.controller_active = controller_active;
    g_ball_balance_status.measured = measured;
    g_ball_balance_status.tracked = tracked;
    g_ball_balance_status.settled = settled;
    g_ball_balance_status.velocity_limit_active = velocity_limit_active;
    g_ball_balance_status.minimum_move_active = g_minimum_move_active;
    g_ball_balance_status.minimum_move_cooldown_active =
        g_minimum_move_cooldown_active;
    g_ball_balance_status.negative_return_assist_active =
        g_negative_return_assist_active;
    g_ball_balance_status.flags = sample.flags;
    g_ball_balance_status.sequence = sample.sequence;
    g_ball_balance_status.confidence_milli = sample.confidence_milli;
    g_ball_balance_status.raw_position_mm = sample.position_mm;
    g_ball_balance_status.raw_velocity_mm_s = sample.velocity_mm_s;
    g_ball_balance_status.camera_timestamp_ms = sample.timestamp_ms;
    g_ball_balance_status.received_at_ms = sample.received_at_ms;
    g_ball_balance_status.accepted_packet_count =
        g_accepted_packet_count;
    g_ball_balance_status.crc_error_count =
        g_crc_error_count;
    g_ball_balance_status.format_error_count =
        g_format_error_count;
    g_ball_balance_status.sequence_drop_count =
        g_sequence_drop_count;
    g_ball_balance_status.filtered_position_mm =
        g_filtered_position_mm;
    g_ball_balance_status.filtered_velocity_mm_s =
        g_filtered_velocity_mm_s;
    g_ball_balance_status.estimated_acceleration_mm_s2 =
        g_filtered_acceleration_mm_s2;
    g_ball_balance_status.position_error_mm = position_error;
    g_ball_balance_status.position_gain_scale = g_position_gain_scale;
    g_ball_balance_status.target_velocity_mm_s =
        g_target_velocity_mm_s;
    g_ball_balance_status.velocity_error_mm_s = velocity_error_mm_s;
    g_ball_balance_status.target_acceleration_mm_s2 =
        target_acceleration_mm_s2;
    g_ball_balance_status.velocity_limit_brake_acceleration_mm_s2 =
        velocity_limit_brake_acceleration_mm_s2;
    g_ball_balance_status.acceleration_error_mm_s2 =
        acceleration_error_mm_s2;
    g_ball_balance_status.acceleration_limit_mm_s2 =
        acceleration_limit_mm_s2;
    g_ball_balance_status.beam_angle_command_deg = beam_angle_deg;
    g_ball_balance_status.servo_linkage_gain = servo_linkage_gain;
    g_ball_balance_status.servo_urgency = servo_urgency;
    g_ball_balance_status.servo_angle_command_deg =
        g_last_servo_angle_deg;
    g_ball_balance_status.servo_effective_angle_limit_deg =
        servo_effective_angle_limit_deg;
    g_ball_balance_status.velocity_integral_servo_deg =
        g_velocity_integral_servo_deg;
    g_ball_balance_status.launch_compensation_servo_deg =
        launch_compensation_servo_deg;
    g_ball_balance_status.vehicle_feedforward_enabled =
        g_vehicle_feedforward_enabled;
    g_ball_balance_status.vehicle_measured_acceleration_mm_s2 =
        g_vehicle_measured_acceleration_mm_s2;
    g_ball_balance_status.vehicle_command_acceleration_mm_s2 =
        g_vehicle_command_acceleration_mm_s2;
    g_ball_balance_status.vehicle_feedforward_acceleration_mm_s2 =
        g_vehicle_feedforward_acceleration_mm_s2;
    g_ball_balance_status.vehicle_feedforward_servo_deg =
        vehicle_feedforward_servo_deg;
    g_ball_balance_status.vehicle_yaw_rate_dps =
        g_vehicle_filtered_yaw_rate_dps;
    g_ball_balance_status.vehicle_turn_compensation_servo_deg =
        vehicle_turn_compensation_servo_deg;
    g_ball_balance_status.rack_travel_command_mm =
        rack_travel_command_mm;
    g_ball_balance_status.breakaway_acceleration_mm_s2 =
        BallBalance_CalculateBreakawayAcceleration();
    g_ball_balance_status.minimum_move_acceleration_mm_s2 =
        g_minimum_move_acceleration_mm_s2;
    g_ball_balance_status.minimum_move_servo_angle_deg =
        g_minimum_move_servo_angle_deg;
    g_ball_balance_status.minimum_move_elapsed_ms =
        g_minimum_move_elapsed_ms;
    g_ball_balance_status.servo_speed_command = servo_speed_command;
    g_ball_balance_status.motion_phase = g_motion_phase;
    g_ball_balance_status.profile_peak_velocity_mm_s =
        g_profile_peak_velocity_mm_s;
    g_ball_balance_status.profile_braking_velocity_mm_s =
        g_profile_braking_velocity_mm_s;
}

void BallBalance_SetTargetMm(float target_mm)
{
    float clamped_target = BallBalance_ClampFloat(
        target_mm, -BALL_TARGET_LIMIT_MM, BALL_TARGET_LIMIT_MM);

    if (BallBalance_AbsFloat(
            clamped_target - g_ball_balance_target_mm) > 0.001f) {
        BallBalance_ResetCascadeState();
        g_task3_negative_target_reached = false;
    }
    g_ball_balance_target_mm = clamped_target;
}

void BallBalance_SetTargetHoldBiasDeg(float bias_deg)
{
    g_target_hold_bias_deg = BallBalance_ClampFloat(
        bias_deg, -BALL_SERVO_MAX_ANGLE_DEG, BALL_SERVO_MAX_ANGLE_DEG);
    g_target_hold_tune_enabled =
        BallBalance_AbsFloat(g_target_hold_bias_deg) > 0.001f;
}

void BallBalance_SetPositionGainScale(float scale)
{
    float clamped_scale = BallBalance_ClampFloat(
        scale,
        BALL_POSITION_GAIN_SCALE_MIN,
        BALL_POSITION_GAIN_SCALE_MAX);

    if (BallBalance_AbsFloat(
            clamped_scale - g_position_gain_scale) > 0.001f) {
        BallBalance_ResetCascadeState();
    }
    g_position_gain_scale = clamped_scale;
}

void BallBalance_SetServoDirection(int8_t direction)
{
    g_ball_balance_servo_direction =
        (direction < 0) ? BALL_BALANCE_SERVO_DIRECTION_REVERSED :
                          BALL_BALANCE_SERVO_DIRECTION_SAME;
}

void BallBalance_SetEnabled(bool enable)
{
    if (g_ball_balance_enabled != enable) {
        BallBalance_ResetCascadeState();
    }
    g_ball_balance_enabled = enable;
}

void BallBalance_SetVehicleFeedforwardEnabled(bool enable)
{
    if (g_vehicle_feedforward_enabled != enable) {
        BallBalance_ResetVehicleMotion();
    }
    g_vehicle_feedforward_enabled = enable;
    g_ball_balance_status.vehicle_feedforward_enabled = enable;
}

void BallBalance_SetVehicleBraking(bool enable)
{
    g_vehicle_braking_active = enable;
}

void BallBalance_SetControlProfile(BallBalanceControlProfile profile)
{
    g_task3_control_profile_active =
        (profile == BALL_BALANCE_PROFILE_TASK3);
    g_task3_negative_target_reached = false;
    g_target_hold_bias_deg = 0.0f;
    g_target_hold_tune_enabled = false;
    if (profile == BALL_BALANCE_PROFILE_TASK4) {
        BallBalance_ApplyTask4Tune();
    } else if (profile == BALL_BALANCE_PROFILE_TASK5) {
        BallBalance_ApplyTask5Tune();
    } else if (profile == BALL_BALANCE_PROFILE_TASK6) {
        BallBalance_ApplyTask6Tune();
    } else if (profile == BALL_BALANCE_PROFILE_TASK7) {
        BallBalance_ApplyTask7Tune();
    } else if (profile == BALL_BALANCE_PROFILE_TASK3) {
        BallBalance_ApplyTask3Tune();
    } else {
        BallBalance_ApplyConservativeTune();
    }

    /* Do not carry velocity targets or learned servo bias across profiles. */
    BallBalance_ResetCascadeState();
}

void BallBalance_UpdateVehicleMotionCmps(float measured_speed_cmps,
                                         float command_speed_cmps,
                                         float yaw_rate_dps)
{
    float acceleration_limit;
    float measured_raw_acceleration_mm_s2;
    float command_raw_acceleration_mm_s2;
    float measured_alpha;
    float command_alpha;
    float command_weight;
    float command_lead_weight;
    float measured_takeover_ratio;
    float command_acceleration_abs;
    float measured_acceleration_abs;
    float measured_takeover_threshold;
    float command_lead_blend;
    float acceleration_deadband;
    float servo_limit;
    float servo_linkage_gain;
    float feedforward_beam_angle_deg;
    float feedforward_target_servo_deg;
    float feedforward_step_deg;
    float feedforward_slew_deg_s;
    float launch_detect_speed_cmps;
    float launch_settle_speed_error_cmps;
    float launch_settle_acceleration_mm_s2;
    float launch_hold_ratio;
    uint32_t launch_hold_max_ms;
    float command_motion_direction;
    float measured_acceleration_along_motion;
    float speed_tracking_error_cmps;
    float command_acceleration_along_motion;
    float turn_alpha;
    float turn_deadband_dps;
    float turn_rate_magnitude_dps;
    float turn_target_servo_deg;
    float turn_servo_limit_deg;
    float turn_servo_step_deg;
    float direction;
    bool launch_stable;
    bool launch_braking;
    bool measured_speed_updated;
    bool command_speed_updated;

    g_ball_balance_status.vehicle_measured_speed_cmps = measured_speed_cmps;
    g_ball_balance_status.vehicle_command_speed_cmps = command_speed_cmps;
    if (!g_vehicle_feedforward_enabled) {
        BallBalance_ResetVehicleMotion();
        return;
    }

    turn_alpha = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_turn_compensation_filter_alpha,
        0.0f, 1.0f);
    g_vehicle_filtered_yaw_rate_dps += turn_alpha *
        (yaw_rate_dps - g_vehicle_filtered_yaw_rate_dps);
    turn_deadband_dps = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_turn_compensation_deadband_dps);
    turn_rate_magnitude_dps = BallBalance_AbsFloat(
        g_vehicle_filtered_yaw_rate_dps);
    if (turn_rate_magnitude_dps > turn_deadband_dps) {
        turn_target_servo_deg =
            ((g_vehicle_filtered_yaw_rate_dps < 0.0f) ? -1.0f : 1.0f) *
            (turn_rate_magnitude_dps - turn_deadband_dps) *
            BallBalance_AbsFloat(
                g_ball_balance_config.
                    vehicle_turn_compensation_gain_deg_per_dps);
    } else {
        turn_target_servo_deg = 0.0f;
    }
    turn_servo_limit_deg = BallBalance_MinFloat(
        BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_turn_compensation_limit_deg),
        BALL_SERVO_MAX_ANGLE_DEG);
    turn_target_servo_deg = BallBalance_ClampFloat(
        turn_target_servo_deg,
        -turn_servo_limit_deg, turn_servo_limit_deg);
    turn_servo_step_deg = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_turn_compensation_slew_deg_s) *
        BALL_CONTROL_PERIOD_S;
    g_vehicle_turn_compensation_servo_deg += BallBalance_ClampFloat(
        turn_target_servo_deg - g_vehicle_turn_compensation_servo_deg,
        -turn_servo_step_deg, turn_servo_step_deg);

    if (!g_vehicle_motion_initialized) {
        g_vehicle_previous_measured_speed_cmps = measured_speed_cmps;
        /* When the chassis is still stopped, retain the initial 0 -> launch
         * command step instead of discarding it during initialization.  This
         * provides feedforward before encoder differentiation catches up. */
        if ((BallBalance_AbsFloat(measured_speed_cmps) < 1.0f) &&
            (BallBalance_AbsFloat(command_speed_cmps) > 0.5f)) {
            g_vehicle_previous_command_speed_cmps = 0.0f;
        } else {
            g_vehicle_previous_command_speed_cmps = command_speed_cmps;
        }
        g_vehicle_measured_sample_elapsed_s = 0.0f;
        g_vehicle_command_sample_elapsed_s = 0.0f;
        g_vehicle_measured_filter_decay_elapsed_s = 0.0f;
        g_vehicle_command_filter_decay_elapsed_s = 0.0f;
        g_vehicle_motion_initialized = true;
        if (g_ball_balance_config.vehicle_launch_preload_enabled &&
            g_vehicle_launch_armed &&
            (BallBalance_AbsFloat(measured_speed_cmps) < 1.0f) &&
            (BallBalance_AbsFloat(command_speed_cmps) >= BallBalance_AbsFloat(
                g_ball_balance_config.vehicle_launch_detect_speed_cmps))) {
            float preload_direction =
                (g_ball_balance_config.vehicle_feedforward_direction < 0) ?
                -1.0f : 1.0f;

            preload_direction *= (command_speed_cmps < 0.0f) ? -1.0f : 1.0f;
            g_vehicle_launch_armed = false;
            g_vehicle_launch_hold_active = true;
            g_vehicle_launch_hold_servo_deg = preload_direction *
                BallBalance_MinFloat(
                    BallBalance_AbsFloat(g_ball_balance_config.
                        vehicle_feedforward_servo_limit_deg),
                    BALL_SERVO_MAX_ANGLE_DEG);
            g_vehicle_launch_motion_direction =
                (command_speed_cmps < 0.0f) ? -1.0f : 1.0f;
            g_vehicle_launch_elapsed_s = 0.0f;
            g_vehicle_launch_stable_elapsed_s = 0.0f;
        }
        return;
    }

    acceleration_limit = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_acceleration_limit_mm_s2);
    g_vehicle_measured_sample_elapsed_s += BALL_CONTROL_PERIOD_S;
    g_vehicle_command_sample_elapsed_s += BALL_CONTROL_PERIOD_S;
    g_vehicle_measured_filter_decay_elapsed_s += BALL_CONTROL_PERIOD_S;
    g_vehicle_command_filter_decay_elapsed_s += BALL_CONTROL_PERIOD_S;
    measured_speed_updated = BallBalance_AbsFloat(
        measured_speed_cmps - g_vehicle_previous_measured_speed_cmps) > 0.001f;
    if (measured_speed_updated) {
        measured_raw_acceleration_mm_s2 =
            (measured_speed_cmps - g_vehicle_previous_measured_speed_cmps) *
            10.0f / BallBalance_MaxFloat(
                g_vehicle_measured_sample_elapsed_s, BALL_CONTROL_PERIOD_S);
        g_vehicle_previous_measured_speed_cmps = measured_speed_cmps;
        g_vehicle_measured_sample_elapsed_s = 0.0f;
        g_vehicle_measured_filter_decay_elapsed_s = 0.0f;
    } else {
        measured_raw_acceleration_mm_s2 = 0.0f;
    }
    command_speed_updated = BallBalance_AbsFloat(
        command_speed_cmps - g_vehicle_previous_command_speed_cmps) > 0.001f;
    if (command_speed_updated) {
        command_raw_acceleration_mm_s2 =
            (command_speed_cmps - g_vehicle_previous_command_speed_cmps) *
            10.0f / BallBalance_MaxFloat(
                g_vehicle_command_sample_elapsed_s, BALL_CONTROL_PERIOD_S);
        g_vehicle_previous_command_speed_cmps = command_speed_cmps;
        g_vehicle_command_sample_elapsed_s = 0.0f;
        g_vehicle_command_filter_decay_elapsed_s = 0.0f;
    } else {
        command_raw_acceleration_mm_s2 = 0.0f;
    }

    measured_raw_acceleration_mm_s2 = BallBalance_ClampFloat(
        measured_raw_acceleration_mm_s2,
        -acceleration_limit, acceleration_limit);
    command_raw_acceleration_mm_s2 = BallBalance_ClampFloat(
        command_raw_acceleration_mm_s2,
        -acceleration_limit, acceleration_limit);
    measured_alpha = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_measured_acceleration_filter_alpha,
        0.0f, 1.0f);
    command_alpha = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_command_acceleration_filter_alpha,
        0.0f, 1.0f);
    if (measured_speed_updated) {
        g_vehicle_measured_acceleration_filter_mm_s2 += measured_alpha *
            (measured_raw_acceleration_mm_s2 -
             g_vehicle_measured_acceleration_filter_mm_s2);
    } else if (g_vehicle_measured_filter_decay_elapsed_s >= 0.040f) {
        g_vehicle_measured_acceleration_filter_mm_s2 += measured_alpha *
            (0.0f - g_vehicle_measured_acceleration_filter_mm_s2);
        g_vehicle_measured_filter_decay_elapsed_s = 0.0f;
    }
    if (command_speed_updated) {
        g_vehicle_command_acceleration_filter_mm_s2 += command_alpha *
            (command_raw_acceleration_mm_s2 -
             g_vehicle_command_acceleration_filter_mm_s2);
    } else if (g_vehicle_command_filter_decay_elapsed_s >= 0.040f) {
        g_vehicle_command_acceleration_filter_mm_s2 += command_alpha *
            (0.0f - g_vehicle_command_acceleration_filter_mm_s2);
        g_vehicle_command_filter_decay_elapsed_s = 0.0f;
    }

    /* The deadband applies to the control output, not filter memory.  Keeping
     * the filtered value allows a smooth low acceleration to accumulate past
     * the deadband instead of being reset to zero every 5 ms. */
    g_vehicle_measured_acceleration_mm_s2 =
        g_vehicle_measured_acceleration_filter_mm_s2;
    g_vehicle_command_acceleration_mm_s2 =
        g_vehicle_command_acceleration_filter_mm_s2;

    acceleration_deadband = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_acceleration_deadband_mm_s2);
    if (BallBalance_AbsFloat(g_vehicle_measured_acceleration_mm_s2) <
        acceleration_deadband) {
        g_vehicle_measured_acceleration_mm_s2 = 0.0f;
    }
    if (BallBalance_AbsFloat(g_vehicle_command_acceleration_mm_s2) <
        acceleration_deadband) {
        g_vehicle_command_acceleration_mm_s2 = 0.0f;
    }

    command_weight = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_command_acceleration_weight,
        0.0f, 1.0f);
    command_lead_weight = BallBalance_MaxFloat(
        command_weight,
        BallBalance_ClampFloat(
            g_ball_balance_config.vehicle_command_acceleration_lead_weight,
            0.0f, 1.0f));
    measured_takeover_ratio = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_measured_acceleration_takeover_ratio,
        0.05f, 1.0f);
    command_acceleration_abs = BallBalance_AbsFloat(
        g_vehicle_command_acceleration_mm_s2);
    measured_acceleration_abs = BallBalance_AbsFloat(
        g_vehicle_measured_acceleration_mm_s2);

    /* Encoder acceleration is delayed because wheel speed must change before
     * it can be differentiated.  Lead with the command, then continuously
     * hand authority to the measured acceleration as it catches up. */
    if (command_acceleration_abs > acceleration_deadband) {
        measured_takeover_threshold = command_acceleration_abs *
            measured_takeover_ratio;
        if ((g_vehicle_command_acceleration_mm_s2 *
             g_vehicle_measured_acceleration_mm_s2) <= 0.0f) {
            command_lead_blend = 1.0f;
        } else {
            command_lead_blend = 1.0f - BallBalance_ClampFloat(
                measured_acceleration_abs /
                    BallBalance_MaxFloat(
                        measured_takeover_threshold,
                        acceleration_deadband),
                0.0f, 1.0f);
        }
        command_weight += (command_lead_weight - command_weight) *
            BallBalance_Smoothstep01(command_lead_blend);
    }
    g_vehicle_feedforward_acceleration_mm_s2 =
        BallBalance_AbsFloat(g_ball_balance_config.vehicle_feedforward_gain) *
        (command_weight * g_vehicle_command_acceleration_mm_s2 +
         (1.0f - command_weight) *
             g_vehicle_measured_acceleration_mm_s2);
    g_vehicle_feedforward_acceleration_mm_s2 = BallBalance_ClampFloat(
        g_vehicle_feedforward_acceleration_mm_s2,
        -acceleration_limit, acceleration_limit);

    direction =
        (g_ball_balance_config.vehicle_feedforward_direction < 0) ?
        -1.0f : 1.0f;
    servo_limit = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_feedforward_servo_limit_deg);
    servo_limit = BallBalance_MinFloat(
        servo_limit, BALL_SERVO_MAX_ANGLE_DEG);
    if (BallBalance_AbsFloat(g_ball_balance_config.servo_gear_radius_mm) >
        0.001f) {
        servo_linkage_gain = BallBalance_AbsFloat(
            g_ball_balance_config.beam_length_mm) /
            BallBalance_AbsFloat(
                g_ball_balance_config.servo_gear_radius_mm);
    } else {
        servo_linkage_gain = 0.0f;
    }
    feedforward_beam_angle_deg = BallBalance_AccelerationToBeamAngle(
        g_vehicle_feedforward_acceleration_mm_s2);
    feedforward_target_servo_deg = direction *
        feedforward_beam_angle_deg * servo_linkage_gain;
    feedforward_target_servo_deg = BallBalance_ClampFloat(
        feedforward_target_servo_deg, -servo_limit, servo_limit);
    if (g_target_hold_tune_enabled && g_vehicle_braking_active) {
        /* Task 7 parking data showed a -14.7 degree deceleration feedforward
         * combining with breakaway control and producing a +28 degree output.
         * Keep launch feedforward unchanged and bound only the braking side. */
        feedforward_target_servo_deg = BallBalance_MaxFloat(
            feedforward_target_servo_deg,
            BALL_TARGET_HOLD_BRAKING_FEEDFORWARD_MIN_DEG);
    }

    launch_detect_speed_cmps = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_launch_detect_speed_cmps);
    launch_settle_speed_error_cmps = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_launch_settle_speed_error_cmps);
    launch_settle_acceleration_mm_s2 = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_launch_settle_acceleration_mm_s2);
    launch_hold_ratio = BallBalance_ClampFloat(
        g_ball_balance_config.vehicle_launch_hold_ratio, 0.0f, 1.0f);
    launch_hold_max_ms =
        (uint32_t)g_ball_balance_config.vehicle_launch_hold_max_ms;
    if (g_target_hold_tune_enabled) {
        launch_hold_ratio = 1.0f;
        launch_hold_max_ms = BALL_TARGET_HOLD_LAUNCH_HOLD_MAX_MS;
    }
    command_motion_direction = (command_speed_cmps < 0.0f) ? -1.0f : 1.0f;
    command_acceleration_along_motion =
        g_vehicle_command_acceleration_mm_s2 * command_motion_direction;
    measured_acceleration_along_motion =
        g_vehicle_measured_acceleration_mm_s2 * command_motion_direction;
    if (BallBalance_AbsFloat(command_speed_cmps) <
        (0.5f * launch_detect_speed_cmps)) {
        g_vehicle_launch_armed = true;
    }

    if (g_vehicle_launch_armed && !g_vehicle_launch_hold_active &&
        (BallBalance_AbsFloat(command_speed_cmps) >=
         launch_detect_speed_cmps) &&
        ((command_acceleration_along_motion > acceleration_deadband) ||
         (measured_acceleration_along_motion > acceleration_deadband)) &&
        (BallBalance_AbsFloat(feedforward_target_servo_deg) > 0.05f)) {
        g_vehicle_launch_armed = false;
        g_vehicle_launch_hold_active = true;
        if (g_ball_balance_config.vehicle_launch_preload_enabled) {
            g_vehicle_launch_hold_servo_deg = direction *
                command_motion_direction * servo_limit;
        } else {
            g_vehicle_launch_hold_servo_deg = feedforward_target_servo_deg;
        }
        g_vehicle_launch_motion_direction = command_motion_direction;
        g_vehicle_launch_elapsed_s = 0.0f;
        g_vehicle_launch_stable_elapsed_s = 0.0f;
    }

    if (g_vehicle_launch_hold_active) {
        /* Task 4 starts its hold timer at real wheel motion, not at the speed
         * command. Static-friction delay varied from 0.1 to 1.3 s in logs. */
        if (g_ball_balance_config.vehicle_launch_preload_enabled &&
            (BallBalance_AbsFloat(measured_speed_cmps) <
             launch_detect_speed_cmps)) {
            g_vehicle_launch_elapsed_s = 0.0f;
        } else {
            g_vehicle_launch_elapsed_s += BALL_CONTROL_PERIOD_S;
        }
        if ((feedforward_target_servo_deg *
             g_vehicle_launch_hold_servo_deg) > 0.0f &&
            (BallBalance_AbsFloat(feedforward_target_servo_deg) >
             BallBalance_AbsFloat(g_vehicle_launch_hold_servo_deg))) {
            g_vehicle_launch_hold_servo_deg = feedforward_target_servo_deg;
        }

        speed_tracking_error_cmps = BallBalance_AbsFloat(
            command_speed_cmps - measured_speed_cmps);
        launch_stable =
            (speed_tracking_error_cmps <= launch_settle_speed_error_cmps) &&
            (BallBalance_AbsFloat(g_vehicle_measured_acceleration_mm_s2) <=
             launch_settle_acceleration_mm_s2) &&
            (BallBalance_AbsFloat(g_vehicle_command_acceleration_mm_s2) <=
             launch_settle_acceleration_mm_s2);
        if (launch_stable) {
            g_vehicle_launch_stable_elapsed_s += BALL_CONTROL_PERIOD_S;
        } else {
            g_vehicle_launch_stable_elapsed_s = 0.0f;
        }

        launch_braking =
            ((command_speed_cmps * g_vehicle_launch_motion_direction) <
             -launch_detect_speed_cmps) ||
            ((g_vehicle_command_acceleration_mm_s2 *
              g_vehicle_launch_motion_direction) < -acceleration_deadband);
        if (launch_braking ||
            (g_vehicle_launch_stable_elapsed_s * 1000.0f >=
             (float)g_ball_balance_config.vehicle_launch_settle_ms) ||
            (g_vehicle_launch_elapsed_s * 1000.0f >=
             (float)launch_hold_max_ms)) {
            g_vehicle_launch_hold_active = false;
            g_vehicle_launch_stable_elapsed_s = 0.0f;
        }
    }

    if (g_vehicle_launch_hold_active) {
        float held_servo_deg =
            g_vehicle_launch_hold_servo_deg * launch_hold_ratio;

        if (g_ball_balance_config.vehicle_launch_preload_enabled) {
            bool encoder_launch_confirmed =
                (BallBalance_AbsFloat(measured_speed_cmps) >=
                 launch_detect_speed_cmps) ||
                (measured_acceleration_along_motion > acceleration_deadband);

            /* Keep a fixed forward preload through zero-speed encoder gaps.
             * Before actual motion, also cap the command impulse at this value.
             * Once motion is confirmed, sustained acceleration may increase it. */
            if (!encoder_launch_confirmed ||
                ((feedforward_target_servo_deg * held_servo_deg) <= 0.0f) ||
                (BallBalance_AbsFloat(feedforward_target_servo_deg) <
                 BallBalance_AbsFloat(held_servo_deg))) {
                feedforward_target_servo_deg = held_servo_deg;
            }
        } else if (g_target_hold_tune_enabled) {
            /* Encoder acceleration arrives in pulses while wheel speed is
             * still below the ramp command. Preserve the strongest launch
             * compensation between those pulses; a reversed target remains
             * free to brake immediately. */
            if ((feedforward_target_servo_deg * held_servo_deg) >= 0.0f &&
                (BallBalance_AbsFloat(feedforward_target_servo_deg) <
                 BallBalance_AbsFloat(held_servo_deg))) {
                feedforward_target_servo_deg = held_servo_deg;
            }
        } else if ((feedforward_target_servo_deg * held_servo_deg) > 0.0f &&
                   (BallBalance_AbsFloat(feedforward_target_servo_deg) >
                    BallBalance_AbsFloat(held_servo_deg))) {
            /* Preserve the proven behavior for tasks 5, 6 and memorized
             * task-7 targets. Task 4 uses the preload-floor branch above. */
            feedforward_target_servo_deg = held_servo_deg;
        }
    }

    feedforward_slew_deg_s = BallBalance_AbsFloat(
        g_ball_balance_config.vehicle_feedforward_servo_slew_deg_s);
    if (g_vehicle_launch_hold_active &&
        g_ball_balance_config.vehicle_launch_preload_enabled &&
        (BallBalance_AbsFloat(g_ball_balance_config.
            vehicle_launch_feedforward_slew_deg_s) > 0.001f)) {
        feedforward_slew_deg_s = BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_launch_feedforward_slew_deg_s);
    } else if (!g_vehicle_launch_hold_active &&
        ((g_vehicle_feedforward_servo_deg * feedforward_target_servo_deg) >=
         0.0f) &&
        (BallBalance_AbsFloat(feedforward_target_servo_deg) <
         BallBalance_AbsFloat(g_vehicle_feedforward_servo_deg))) {
        feedforward_slew_deg_s = BallBalance_AbsFloat(
            g_ball_balance_config.vehicle_feedforward_release_slew_deg_s);
    }
    feedforward_step_deg = feedforward_slew_deg_s * BALL_CONTROL_PERIOD_S;
    g_vehicle_feedforward_servo_deg += BallBalance_ClampFloat(
        feedforward_target_servo_deg - g_vehicle_feedforward_servo_deg,
        -feedforward_step_deg, feedforward_step_deg);
    g_vehicle_feedforward_servo_deg = BallBalance_ClampFloat(
        g_vehicle_feedforward_servo_deg, -servo_limit, servo_limit);
    if (g_target_hold_tune_enabled && g_vehicle_braking_active) {
        g_vehicle_feedforward_servo_deg = BallBalance_MaxFloat(
            g_vehicle_feedforward_servo_deg,
            BALL_TARGET_HOLD_BRAKING_FEEDFORWARD_MIN_DEG);
    }
}
