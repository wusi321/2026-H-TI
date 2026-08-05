#include "task_controller.h"

#include <stddef.h>
#include <string.h>


static int32_t abs_i16_to_i32(int16_t value) {
    int32_t widened = value;
    return (widened < 0) ? -widened : widened;
}


static int16_t clamp_i16(int16_t value, int16_t lower, int16_t upper) {
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}


static bool observation_is_current(
    const ball_task_controller_t *controller,
    const ball_observation_t *observation,
    uint32_t now_ms
) {
    if (controller == NULL || observation == NULL || !observation->valid) {
        return false;
    }
    return (uint32_t)(now_ms - observation->received_ms) <=
        controller->config.vision_timeout_ms;
}


static uint32_t task_deadline_ms(ball_task_id_t task_id) {
    switch (task_id) {
        case BALL_TASK_2:
            return 20000u;
        case BALL_TASK_3:
            return 5000u;
        case BALL_TASK_4:
            return 8000u;
        case BALL_TASK_5:
        case BALL_TASK_6:
            return 30000u;
        default:
            return 0u;
    }
}


static ball_route_mode_t route_for_task(ball_task_id_t task_id) {
    switch (task_id) {
        case BALL_TASK_2:
            return BALL_ROUTE_ONE_LAP_STOP_A;
        case BALL_TASK_4:
            return BALL_ROUTE_TO_B;
        case BALL_TASK_5:
        case BALL_TASK_6:
            return BALL_ROUTE_ONE_LAP_PASS_A;
        default:
            return BALL_ROUTE_STOP;
    }
}


static void reset_stable_timer(ball_task_controller_t *controller) {
    controller->stable_timer_active = false;
    controller->stable_start_ms = 0u;
}


static void set_phase_target(
    ball_task_controller_t *controller,
    ball_task_phase_t phase,
    int16_t target_mm
) {
    controller->phase = phase;
    controller->target_mm = target_mm;
    reset_stable_timer(controller);
}


void ball_task_default_config(ball_task_config_t *config) {
    if (config == NULL) {
        return;
    }
    config->position_tolerance_mm = 8;
    config->velocity_tolerance_mm_s = 50;
    config->positive_target_mm = 50;
    config->negative_target_mm = -50;
    config->ball_center_limit_mm = 120;
    config->settle_time_ms = 250u;
    config->vision_timeout_ms = 100u;
}


void ball_task_init(
    ball_task_controller_t *controller,
    const ball_task_config_t *config
) {
    ball_task_config_t defaults;
    if (controller == NULL) {
        return;
    }
    memset(controller, 0, sizeof(*controller));
    if (config == NULL) {
        ball_task_default_config(&defaults);
        controller->config = defaults;
    } else {
        controller->config = *config;
    }
    controller->task_id = BALL_TASK_IDLE;
    controller->phase = BALL_PHASE_IDLE;
}


bool ball_task_start(
    ball_task_controller_t *controller,
    ball_task_id_t task_id,
    const ball_observation_t *observation,
    uint32_t now_ms
) {
    bool current;
    if (controller == NULL || task_id < BALL_TASK_2 || task_id > BALL_TASK_6) {
        return false;
    }

    controller->task_id = task_id;
    controller->task_start_ms = now_ms;
    controller->sequence_complete = false;
    controller->route_complete = false;
    controller->deadline_exceeded = false;
    reset_stable_timer(controller);
    current = observation_is_current(controller, observation, now_ms);

    switch (task_id) {
        case BALL_TASK_2:
        case BALL_TASK_4:
        case BALL_TASK_5:
            set_phase_target(controller, BALL_PHASE_HOLD_CENTER, 0);
            break;
        case BALL_TASK_3:
            set_phase_target(controller, BALL_PHASE_TASK3_TO_CENTER, 0);
            break;
        case BALL_TASK_6:
            if (current) {
                set_phase_target(
                    controller,
                    BALL_PHASE_TASK6_HOLD,
                    clamp_i16(
                        observation->position_mm,
                        (int16_t)-controller->config.ball_center_limit_mm,
                        controller->config.ball_center_limit_mm
                    )
                );
            } else {
                set_phase_target(controller, BALL_PHASE_TASK6_CAPTURE, 0);
            }
            break;
        default:
            return false;
    }
    return true;
}


void ball_task_stop(ball_task_controller_t *controller) {
    if (controller == NULL) {
        return;
    }
    controller->task_id = BALL_TASK_IDLE;
    controller->phase = BALL_PHASE_IDLE;
    controller->target_mm = 0;
    controller->task_start_ms = 0u;
    controller->sequence_complete = false;
    controller->route_complete = false;
    controller->deadline_exceeded = false;
    reset_stable_timer(controller);
}


static bool update_stability(
    ball_task_controller_t *controller,
    const ball_observation_t *observation,
    bool current,
    uint32_t now_ms
) {
    int16_t error;
    bool stable;
    if (!current || observation == NULL) {
        reset_stable_timer(controller);
        return false;
    }
    error = (int16_t)(controller->target_mm - observation->position_mm);
    stable =
        abs_i16_to_i32(error) <= controller->config.position_tolerance_mm &&
        abs_i16_to_i32(observation->velocity_mm_s) <=
            controller->config.velocity_tolerance_mm_s;
    if (!stable) {
        reset_stable_timer(controller);
        return false;
    }
    if (!controller->stable_timer_active) {
        controller->stable_timer_active = true;
        controller->stable_start_ms = now_ms;
        return false;
    }
    return (uint32_t)(now_ms - controller->stable_start_ms) >=
        controller->config.settle_time_ms;
}


static void advance_task3(ball_task_controller_t *controller) {
    switch (controller->phase) {
        case BALL_PHASE_TASK3_TO_CENTER:
            set_phase_target(
                controller,
                BALL_PHASE_TASK3_TO_POSITIVE,
                controller->config.positive_target_mm
            );
            break;
        case BALL_PHASE_TASK3_TO_POSITIVE:
            set_phase_target(
                controller,
                BALL_PHASE_TASK3_TO_NEGATIVE,
                controller->config.negative_target_mm
            );
            break;
        case BALL_PHASE_TASK3_TO_NEGATIVE:
            set_phase_target(
                controller,
                BALL_PHASE_TASK3_HOLD_NEGATIVE,
                controller->config.negative_target_mm
            );
            controller->sequence_complete = true;
            break;
        default:
            break;
    }
}


void ball_task_update(
    ball_task_controller_t *controller,
    const ball_observation_t *observation,
    bool route_complete,
    uint32_t now_ms,
    ball_task_output_t *output
) {
    bool current;
    bool settled;
    uint32_t deadline;
    ball_route_mode_t route_mode;
    if (controller == NULL || output == NULL) {
        return;
    }
    memset(output, 0, sizeof(*output));
    current = observation_is_current(controller, observation, now_ms);

    if (controller->phase == BALL_PHASE_TASK6_CAPTURE && current) {
        set_phase_target(
            controller,
            BALL_PHASE_TASK6_HOLD,
            clamp_i16(
                observation->position_mm,
                (int16_t)-controller->config.ball_center_limit_mm,
                controller->config.ball_center_limit_mm
            )
        );
    }

    settled = update_stability(controller, observation, current, now_ms);
    if (controller->task_id == BALL_TASK_3 && settled) {
        advance_task3(controller);
    }

    if (route_complete) {
        controller->route_complete = true;
    }
    deadline = task_deadline_ms(controller->task_id);
    if (deadline > 0u && (uint32_t)(now_ms - controller->task_start_ms) > deadline) {
        controller->deadline_exceeded = true;
    }

    route_mode = route_for_task(controller->task_id);
    if (controller->route_complete || controller->phase == BALL_PHASE_TASK6_CAPTURE) {
        route_mode = BALL_ROUTE_STOP;
    }

    output->task_id = controller->task_id;
    output->phase = controller->phase;
    output->route_mode = route_mode;
    output->target_mm = controller->target_mm;
    output->position_error_mm = current
        ? (int16_t)(controller->target_mm - observation->position_mm)
        : 0;
    output->balance_enabled = controller->task_id != BALL_TASK_IDLE && current &&
        controller->phase != BALL_PHASE_TASK6_CAPTURE;
    output->request_safe_level = controller->task_id == BALL_TASK_IDLE || !current;
    output->deadline_exceeded = controller->deadline_exceeded;
    output->task_complete = (controller->task_id == BALL_TASK_3)
        ? controller->sequence_complete
        : controller->route_complete;
}
