#ifndef BALL_VISION_LINK_H
#define BALL_VISION_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "task_controller.h"
#include "vision_protocol.h"


typedef struct {
    ball_vision_parser_t parser;
    bool has_sequence;
    uint16_t last_sequence;
} ball_vision_link_t;


void ball_vision_link_init(ball_vision_link_t *link);
void ball_vision_link_reset(ball_vision_link_t *link);
bool ball_vision_link_push(
    ball_vision_link_t *link,
    uint8_t byte,
    uint32_t received_ms,
    ball_observation_t *observation_out,
    ball_vision_packet_t *packet_out
);

#endif
