#include "vision_link.h"

#include <string.h>


static bool sequence_is_newer(uint16_t current, uint16_t previous) {
    uint16_t delta = (uint16_t)(current - previous);
    return delta != 0u && delta < 0x8000u;
}


void ball_vision_link_init(ball_vision_link_t *link) {
    if (link == NULL) {
        return;
    }
    memset(link, 0, sizeof(*link));
    ball_vision_parser_init(&link->parser);
}


void ball_vision_link_reset(ball_vision_link_t *link) {
    ball_vision_link_init(link);
}


bool ball_vision_link_push(
    ball_vision_link_t *link,
    uint8_t byte,
    uint32_t received_ms,
    ball_observation_t *observation_out,
    ball_vision_packet_t *packet_out
) {
    ball_vision_packet_t packet;
    bool valid;
    if (link == NULL || observation_out == NULL) {
        return false;
    }
    if (!ball_vision_parser_push(&link->parser, byte, &packet)) {
        return false;
    }
    if (link->has_sequence &&
        !sequence_is_newer(packet.sequence, link->last_sequence)) {
        return false;
    }

    link->has_sequence = true;
    link->last_sequence = packet.sequence;
    valid = (packet.flags & BALL_VISION_FLAG_VALID) != 0u;
    observation_out->valid = valid;
    observation_out->position_mm = valid ? packet.position_mm : 0;
    observation_out->velocity_mm_s = valid ? packet.velocity_mm_s : 0;
    observation_out->confidence_milli = packet.confidence_milli;
    observation_out->received_ms = received_ms;
    if (packet_out != NULL) {
        *packet_out = packet;
    }
    return true;
}
