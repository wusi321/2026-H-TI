#ifndef BALL_TASK_CONTROLLER_H
#define BALL_TASK_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>


typedef enum {
    BALL_TASK_IDLE = 0,
    BALL_TASK_2 = 2,
    BALL_TASK_3 = 3,
    BALL_TASK_4 = 4,
    BALL_TASK_5 = 5,
    BALL_TASK_6 = 6
} ball_task_id_t;


typedef enum {
    BALL_PHASE_IDLE = 0,
    BALL_PHASE_HOLD_CENTER,
    BALL_PHASE_TASK3_TO_CENTER,
    BALL_PHASE_TASK3_TO_POSITIVE,
    BALL_PHASE_TASK3_TO_NEGATIVE,
    BALL_PHASE_TASK3_HOLD_NEGATIVE,
    BALL_PHASE_TASK6_CAPTURE,
    BALL_PHASE_TASK6_HOLD
} ball_task_phase_t;


typedef enum {
    BALL_ROUTE_STOP = 0,
    BALL_ROUTE_ONE_LAP_STOP_A,
    BALL_ROUTE_TO_B,
    BALL_ROUTE_ONE_LAP_PASS_A
} ball_route_mode_t;


typedef struct {
    bool valid;
    int16_t position_mm;
    int16_t velocity_mm_s;
    uint16_t confidence_milli;
    uint32_t received_ms;
} ball_observation_t;


typedef struct {
    int16_t position_tolerance_mm;
    int16_t velocity_tolerance_mm_s;
    int16_t positive_target_mm;
    int16_t negative_target_mm;
    int16_t ball_center_limit_mm;
    uint16_t settle_time_ms;
    uint16_t vision_timeout_ms;
} ball_task_config_t;


typedef struct {
    ball_task_config_t config;
    ball_task_id_t task_id;
    ball_task_phase_t phase;
    int16_t target_mm;
    uint32_t task_start_ms;
    uint32_t stable_start_ms;
    bool stable_timer_active;
    bool sequence_complete;
    bool route_complete;
    bool deadline_exceeded;
} ball_task_controller_t;


typedef struct {
    ball_task_id_t task_id;
    ball_task_phase_t phase;
    ball_route_mode_t route_mode;
    int16_t target_mm;
    int16_t position_error_mm;
    bool balance_enabled;
    bool request_safe_level;
    bool task_complete;
    bool deadline_exceeded;
} ball_task_output_t;


void ball_task_default_config(ball_task_config_t *config);
void ball_task_init(
    ball_task_controller_t *controller,
    const ball_task_config_t *config
);
bool ball_task_start(
    ball_task_controller_t *controller,
    ball_task_id_t task_id,
    const ball_observation_t *observation,
    uint32_t now_ms
);
void ball_task_stop(ball_task_controller_t *controller);
void ball_task_update(
    ball_task_controller_t *controller,
    const ball_observation_t *observation,
    bool route_complete,
    uint32_t now_ms,
    ball_task_output_t *output
);

#endif
