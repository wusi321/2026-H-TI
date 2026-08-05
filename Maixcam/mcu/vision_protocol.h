#ifndef BALL_VISION_PROTOCOL_H
#define BALL_VISION_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BALL_VISION_MAGIC_0 0xAAu
#define BALL_VISION_MAGIC_1 0x55u
#define BALL_VISION_VERSION 1u
#define BALL_VISION_FLAG_VALID 0x01u
#define BALL_VISION_FLAG_MEASURED 0x02u
#define BALL_VISION_FLAG_TRACKED 0x04u
#define BALL_VISION_FLAG_MASK 0x07u
#define BALL_VISION_INVALID_POSITION_MM INT16_MAX
#define BALL_VISION_PACKET_SIZE 20u

/* Decoded native representation; never DMA wire bytes directly into it. */
typedef struct {
    uint8_t magic[2];
    uint8_t version;
    uint8_t flags;
    uint16_t sequence;
    uint32_t timestamp_ms;
    int16_t position_mm;
    int16_t velocity_mm_s;
    uint16_t confidence_milli;
    uint16_t processing_us;
    uint16_t crc16;
} ball_vision_packet_t;

typedef struct {
    uint8_t bytes[BALL_VISION_PACKET_SIZE];
    size_t index;
} ball_vision_parser_t;

uint16_t ball_vision_crc16(const uint8_t *data, size_t length);
void ball_vision_parser_init(ball_vision_parser_t *parser);
bool ball_vision_parser_push(
    ball_vision_parser_t *parser,
    uint8_t byte,
    ball_vision_packet_t *packet_out
);
bool ball_vision_packet_valid(const ball_vision_packet_t *packet);

#endif
