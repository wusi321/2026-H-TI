#ifndef BALL_BALANCE_H
#define BALL_BALANCE_H

#include <stdbool.h>
#include <stdint.h>

#define BALL_BALANCE_SERVO_DIRECTION_SAME      (1)
#define BALL_BALANCE_SERVO_DIRECTION_REVERSED (-1)

typedef enum {
    BALL_BALANCE_MOTION_HOLD = 0,
    BALL_BALANCE_MOTION_ACCELERATE,
    BALL_BALANCE_MOTION_CRUISE,
    BALL_BALANCE_MOTION_DECELERATE
} BallBalanceMotionPhase;

typedef enum {
    BALL_BALANCE_PROFILE_STATIONARY = 0,
    BALL_BALANCE_PROFILE_TASK4,
    BALL_BALANCE_PROFILE_TASK5,
    BALL_BALANCE_PROFILE_TASK6,
    BALL_BALANCE_PROFILE_TASK7,
    BALL_BALANCE_PROFILE_TASK3
} BallBalanceControlProfile;

typedef struct {
    float position_to_velocity_kp_s;
    float max_target_velocity_mm_s;
    float minimum_target_velocity_mm_s;
    float minimum_target_velocity_full_error_mm;
    float motion_prediction_time_s;
    float velocity_limit_brake_start_ratio;
    float velocity_limit_overspeed_kp_s;
    float velocity_limit_max_brake_mm_s2;
    float braking_acceleration_mm_s2;
    float distance_brake_gain;
    float distance_brake_stop_offset_mm;
    float velocity_reference_accel_limit_mm_s2;
    float motion_phase_velocity_hysteresis_mm_s;
    float velocity_to_acceleration_kp_s;
    float acceleration_feedforward_gain;
    float acceleration_limit_near_mm_s2;
    float acceleration_limit_far_mm_s2;
    float acceleration_limit_brake_mm_s2;
    float acceleration_limit_full_error_mm;
    float acceleration_filter_alpha;
    float gravity_mm_s2;
    float rolling_acceleration_ratio;
    float acceleration_feedback_gain;
    float servo_degrees_per_acceleration_mm_s2;
    float velocity_integral_gain_deg_per_mm;
    float velocity_integral_unwind_gain_deg_per_mm;
    float velocity_integral_limit_deg;
    float velocity_integral_deadband_mm_s;
    float servo_normal_angle_limit_deg;
    float servo_near_target_min_limit_deg;
    float servo_near_target_full_error_mm;
    float servo_near_target_full_brake_velocity_mm_s;
    float task3_positive_servo_limit_deg;
    float task3_positive_target_velocity_mm_s;
    float task3_negative_minimum_move_error_mm;
    float task3_speed_anti_decay_ratio;
    float vehicle_feedforward_gain;
    float vehicle_command_acceleration_weight;
    float vehicle_command_acceleration_lead_weight;
    float vehicle_measured_acceleration_takeover_ratio;
    float vehicle_measured_acceleration_filter_alpha;
    float vehicle_command_acceleration_filter_alpha;
    float vehicle_acceleration_deadband_mm_s2;
    float vehicle_acceleration_limit_mm_s2;
    float vehicle_feedforward_servo_limit_deg;
    float vehicle_feedforward_servo_slew_deg_s;
    float vehicle_feedforward_release_slew_deg_s;
    float vehicle_feedforward_command_reserve_ratio;
    float vehicle_braking_servo_extra_deg;
    float vehicle_braking_servo_extra_error_mm;
    float vehicle_braking_feedforward_preload_error_mm;
    float vehicle_turn_compensation_gain_deg_per_dps;
    float vehicle_turn_compensation_deadband_dps;
    float vehicle_turn_compensation_limit_deg;
    float vehicle_turn_compensation_filter_alpha;
    float vehicle_turn_compensation_slew_deg_s;
    float vehicle_launch_hold_ratio;
    float vehicle_launch_detect_speed_cmps;
    float vehicle_launch_settle_speed_error_cmps;
    float vehicle_launch_settle_acceleration_mm_s2;
    float vehicle_launch_feedforward_slew_deg_s;
    uint16_t vehicle_launch_settle_ms;
    uint16_t vehicle_launch_hold_max_ms;
    bool vehicle_launch_preload_enabled;
    float servo_accel_slew_deg_s;
    float servo_brake_slew_deg_s;
    float servo_level_slew_deg_s;
    float beam_length_mm;
    float servo_gear_radius_mm;
    float breakaway_rack_travel_mm;
    float breakaway_acceleration_margin;
    float settle_position_tolerance_mm;
    float settle_velocity_tolerance_mm_s;
    float servo_neutral_angle_deg;
    float servo_hold_bias_deg;
    float servo_min_angle_deg;
    float servo_max_angle_deg;
    float position_filter_alpha;
    float velocity_filter_alpha;
    float tracked_gain_scale;
    float minimum_move_error_mm;
    float minimum_move_stationary_delta_mm;
    float minimum_move_release_speed_mm_s;
    float minimum_move_acceleration_mm_s2;
    float minimum_move_acceleration_max_mm_s2;
    float minimum_move_acceleration_ramp_mm_s3;
    float minimum_move_servo_start_deg;
    float minimum_move_servo_max_deg;
    float minimum_move_servo_ramp_deg_s;
    float minimum_move_servo_slew_deg_s;
    float negative_near_minimum_move_error_max_mm;
    float negative_near_minimum_move_release_speed_mm_s;
    float negative_near_minimum_move_servo_start_deg;
    float negative_near_minimum_move_servo_max_deg;
    float negative_near_minimum_move_servo_ramp_deg_s;
    float negative_near_minimum_move_servo_slew_deg_s;
    float positive_near_minimum_move_error_max_mm;
    float positive_near_minimum_move_release_speed_mm_s;
    float negative_return_assist_error_max_mm;
    float negative_return_assist_speed_mm_s;
    float negative_return_assist_release_error_mm;
    float negative_return_assist_brake_limit_mm_s2;
    float servo_speed_full_error_mm;
    float servo_speed_full_velocity_mm_s;
    float servo_speed_full_angle_delta_deg;
    uint32_t vision_timeout_ms;
    uint32_t vehicle_braking_vision_timeout_ms;
    uint16_t minimum_move_detect_ms;
    uint16_t minimum_move_cooldown_ms;
    bool launch_compensation_requires_stationary;
    uint16_t minimum_confidence_milli;
    uint16_t servo_speed;
    uint16_t servo_speed_min;
    uint16_t servo_speed_max;
    int8_t vehicle_feedforward_direction;
} BallBalanceConfig;

typedef struct {
    bool protocol_self_test_ok;
    bool vision_valid;
    bool controller_active;
    bool measured;
    bool tracked;
    bool settled;
    bool velocity_limit_active;
    bool minimum_move_active;
    bool minimum_move_cooldown_active;
    bool negative_return_assist_active;
    uint8_t flags;
    uint16_t sequence;
    uint16_t confidence_milli;
    int16_t raw_position_mm;
    int16_t raw_velocity_mm_s;
    uint32_t camera_timestamp_ms;
    uint32_t received_at_ms;
    uint32_t accepted_packet_count;
    uint32_t crc_error_count;
    uint32_t format_error_count;
    uint32_t sequence_drop_count;
    uint32_t timeout_count;
    float filtered_position_mm;
    float filtered_velocity_mm_s;
    float estimated_acceleration_mm_s2;
    float requested_target_mm;
    float target_reference_mm;
    float position_error_mm;
    float position_gain_scale;
    float target_velocity_mm_s;
    float velocity_error_mm_s;
    float target_acceleration_mm_s2;
    float velocity_limit_brake_acceleration_mm_s2;
    float acceleration_error_mm_s2;
    float acceleration_limit_mm_s2;
    float beam_angle_command_deg;
    float servo_linkage_gain;
    float servo_urgency;
    float servo_angle_command_deg;
    float servo_effective_angle_limit_deg;
    float velocity_integral_servo_deg;
    float launch_compensation_servo_deg;
    bool vehicle_feedforward_enabled;
    float vehicle_measured_speed_cmps;
    float vehicle_command_speed_cmps;
    float vehicle_measured_acceleration_mm_s2;
    float vehicle_command_acceleration_mm_s2;
    float vehicle_feedforward_acceleration_mm_s2;
    float vehicle_feedforward_servo_deg;
    float vehicle_yaw_rate_dps;
    float vehicle_turn_compensation_servo_deg;
    float rack_travel_command_mm;
    float breakaway_acceleration_mm_s2;
    float minimum_move_acceleration_mm_s2;
    float minimum_move_servo_angle_deg;
    uint32_t minimum_move_elapsed_ms;
    uint16_t servo_speed_command;
    BallBalanceMotionPhase motion_phase;
    float profile_peak_velocity_mm_s;
    float profile_braking_velocity_mm_s;
} BallBalanceStatus;

extern BallBalanceConfig g_ball_balance_config;
extern BallBalanceStatus g_ball_balance_status;

/*
 * This is only the installed linkage polarity. Motion direction is generated
 * continuously by the position/velocity controller.
 */
extern volatile int8_t g_ball_balance_servo_direction;
extern volatile float g_ball_balance_target_mm;
extern volatile bool g_ball_balance_enabled;

void BallBalance_Init(void);
void BallBalance_UART1_RxByte(uint8_t byte);
void BallBalance_UART1_ResetParser(void);
void BallBalance_Control200Hz(void);
void BallBalance_SetTargetMm(float target_mm);
void BallBalance_SetTargetHoldBiasDeg(float bias_deg);
void BallBalance_SetPositionGainScale(float scale);
void BallBalance_SetServoDirection(int8_t direction);
void BallBalance_SetEnabled(bool enable);
void BallBalance_SetVehicleFeedforwardEnabled(bool enable);
void BallBalance_SetVehicleBraking(bool enable);
void BallBalance_SetControlProfile(BallBalanceControlProfile profile);
void BallBalance_UpdateVehicleMotionCmps(float measured_speed_cmps,
                                         float command_speed_cmps,
                                         float yaw_rate_dps);

#endif
