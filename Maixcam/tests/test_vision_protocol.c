#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vision_link.h"
#include "vision_protocol.h"


static const uint8_t packet_17[BALL_VISION_PACKET_SIZE] = {
    0xAA, 0x55, 0x01, 0x03, 0x11, 0x00, 0x40, 0xE2, 0x01, 0x00,
    0xDE, 0xFF, 0x7B, 0x00, 0x6C, 0x03, 0x68, 0x10, 0xB5, 0xB4
};

static const uint8_t packet_18[BALL_VISION_PACKET_SIZE] = {
    0xAA, 0x55, 0x01, 0x03, 0x12, 0x00, 0x61, 0xE2, 0x01, 0x00,
    0xDE, 0xFF, 0x7B, 0x00, 0x6C, 0x03, 0x68, 0x10, 0xC3, 0x54
};

static const uint8_t invalid_packet[BALL_VISION_PACKET_SIZE] = {
    0xAA, 0x55, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x2C
};


static bool push_packet(
    ball_vision_parser_t *parser,
    const uint8_t packet[BALL_VISION_PACKET_SIZE],
    ball_vision_packet_t *packet_out
) {
    size_t index;
    bool decoded = false;
    for (index = 0u; index < BALL_VISION_PACKET_SIZE; ++index) {
        if (ball_vision_parser_push(parser, packet[index], packet_out)) {
            decoded = true;
        }
    }
    return decoded;
}


static bool push_link_packet(
    ball_vision_link_t *link,
    const uint8_t packet[BALL_VISION_PACKET_SIZE],
    uint32_t received_ms,
    ball_observation_t *observation
) {
    size_t index;
    bool decoded = false;
    for (index = 0u; index < BALL_VISION_PACKET_SIZE; ++index) {
        if (ball_vision_link_push(
                link,
                packet[index],
                received_ms,
                observation,
                NULL
            )) {
            decoded = true;
        }
    }
    return decoded;
}


static void test_golden_packet(void) {
    static const uint8_t check[] = "123456789";
    ball_vision_parser_t parser;
    ball_vision_packet_t packet;
    assert(ball_vision_crc16(check, sizeof(check) - 1u) == 0x29B1u);
    ball_vision_parser_init(&parser);
    assert(push_packet(&parser, packet_17, &packet));
    assert(packet.sequence == 17u);
    assert(packet.timestamp_ms == 123456u);
    assert(packet.position_mm == -34);
    assert(packet.velocity_mm_s == 123);
    assert(packet.confidence_milli == 876u);
    assert(packet.processing_us == 4200u);
}


static void test_resync_after_deleted_byte(void) {
    ball_vision_parser_t parser;
    ball_vision_packet_t packet;
    size_t index;
    bool decoded = false;
    ball_vision_parser_init(&parser);

    for (index = 0u; index < BALL_VISION_PACKET_SIZE; ++index) {
        if (index != 8u) {
            assert(!ball_vision_parser_push(&parser, packet_17[index], &packet));
        }
    }
    for (index = 0u; index < BALL_VISION_PACKET_SIZE; ++index) {
        if (ball_vision_parser_push(&parser, packet_18[index], &packet)) {
            decoded = true;
        }
    }
    assert(decoded);
    assert(packet.sequence == 18u);
}


static void test_link_rejects_duplicate_sequence(void) {
    ball_vision_link_t link;
    ball_observation_t observation;
    ball_vision_link_init(&link);
    memset(&observation, 0, sizeof(observation));

    assert(push_link_packet(&link, packet_17, 1000u, &observation));
    assert(observation.valid);
    assert(observation.received_ms == 1000u);
    assert(!push_link_packet(&link, packet_17, 1010u, &observation));
    assert(observation.received_ms == 1000u);
    assert(push_link_packet(&link, packet_18, 1020u, &observation));
    assert(observation.received_ms == 1020u);
    ball_vision_link_reset(&link);
    assert(push_link_packet(&link, packet_17, 1030u, &observation));
}


static void test_link_accepts_safe_invalid_packet(void) {
    ball_vision_link_t link;
    ball_observation_t observation;
    ball_vision_link_init(&link);
    memset(&observation, 0, sizeof(observation));
    assert(push_link_packet(&link, invalid_packet, 2000u, &observation));
    assert(!observation.valid);
    assert(observation.position_mm == 0);
    assert(observation.velocity_mm_s == 0);
}


int main(void) {
    test_golden_packet();
    test_resync_after_deleted_byte();
    test_link_rejects_duplicate_sequence();
    test_link_accepts_safe_invalid_packet();
    return 0;
}
