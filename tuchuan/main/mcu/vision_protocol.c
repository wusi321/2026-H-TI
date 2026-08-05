#include "vision_protocol.h"

#include <string.h>


#define BALL_VISION_BODY_SIZE (BALL_VISION_PACKET_SIZE - 2u)


static uint16_t read_u16_le(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}


static int16_t read_i16_le(const uint8_t *data) {
    uint16_t value = read_u16_le(data);
    if (value <= (uint16_t)INT16_MAX) {
        return (int16_t)value;
    }
    return (int16_t)((int32_t)value - 65536L);
}


static uint32_t read_u32_le(const uint8_t *data) {
    return (uint32_t)data[0] |
        ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) |
        ((uint32_t)data[3] << 24);
}


static void write_u16_le(uint8_t *data, uint16_t value) {
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)(value >> 8);
}


static void write_u32_le(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)((value >> 8) & 0xFFu);
    data[2] = (uint8_t)((value >> 16) & 0xFFu);
    data[3] = (uint8_t)(value >> 24);
}


static void encode_packet_body(
    const ball_vision_packet_t *packet,
    uint8_t body[BALL_VISION_BODY_SIZE]
) {
    body[0] = packet->magic[0];
    body[1] = packet->magic[1];
    body[2] = packet->version;
    body[3] = packet->flags;
    write_u16_le(&body[4], packet->sequence);
    write_u32_le(&body[6], packet->timestamp_ms);
    write_u16_le(&body[10], (uint16_t)packet->position_mm);
    write_u16_le(&body[12], (uint16_t)packet->velocity_mm_s);
    write_u16_le(&body[14], packet->confidence_milli);
    write_u16_le(&body[16], packet->processing_us);
}


static void decode_packet(
    const uint8_t bytes[BALL_VISION_PACKET_SIZE],
    ball_vision_packet_t *packet
) {
    packet->magic[0] = bytes[0];
    packet->magic[1] = bytes[1];
    packet->version = bytes[2];
    packet->flags = bytes[3];
    packet->sequence = read_u16_le(&bytes[4]);
    packet->timestamp_ms = read_u32_le(&bytes[6]);
    packet->position_mm = read_i16_le(&bytes[10]);
    packet->velocity_mm_s = read_i16_le(&bytes[12]);
    packet->confidence_milli = read_u16_le(&bytes[14]);
    packet->processing_us = read_u16_le(&bytes[16]);
    packet->crc16 = read_u16_le(&bytes[18]);
}


static void parser_resync(ball_vision_parser_t *parser) {
    size_t start;
    for (start = 1u; start + 1u < parser->index; ++start) {
        if (parser->bytes[start] == BALL_VISION_MAGIC_0 &&
            parser->bytes[start + 1u] == BALL_VISION_MAGIC_1) {
            memmove(
                parser->bytes,
                &parser->bytes[start],
                parser->index - start
            );
            parser->index -= start;
            return;
        }
    }
    if (parser->index > 0u &&
        parser->bytes[parser->index - 1u] == BALL_VISION_MAGIC_0) {
        parser->bytes[0] = BALL_VISION_MAGIC_0;
        parser->index = 1u;
    } else {
        parser->index = 0u;
    }
}


uint16_t ball_vision_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFFu;
    size_t index;
    int bit;
    for (index = 0; index < length; ++index) {
        crc ^= (uint16_t)data[index] << 8;
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}


bool ball_vision_packet_valid(const ball_vision_packet_t *packet) {
    uint8_t body[BALL_VISION_BODY_SIZE];
    uint16_t expected;
    if (packet == NULL) {
        return false;
    }
    if (packet->magic[0] != BALL_VISION_MAGIC_0 ||
        packet->magic[1] != BALL_VISION_MAGIC_1 ||
        packet->version != BALL_VISION_VERSION) {
        return false;
    }
    if ((packet->flags & (uint8_t)~BALL_VISION_FLAG_MASK) != 0u ||
        packet->confidence_milli > 1000u) {
        return false;
    }
    if ((packet->flags & BALL_VISION_FLAG_VALID) == 0u) {
        if ((packet->flags &
             (BALL_VISION_FLAG_MEASURED | BALL_VISION_FLAG_TRACKED)) != 0u ||
            packet->position_mm != BALL_VISION_INVALID_POSITION_MM ||
            packet->velocity_mm_s != 0) {
            return false;
        }
    } else if ((packet->flags & BALL_VISION_FLAG_MEASURED) != 0u &&
               (packet->flags & BALL_VISION_FLAG_TRACKED) != 0u) {
        return false;
    }
    encode_packet_body(packet, body);
    expected = ball_vision_crc16(body, BALL_VISION_BODY_SIZE);
    return expected == packet->crc16;
}


void ball_vision_parser_init(ball_vision_parser_t *parser) {
    if (parser != NULL) {
        memset(parser, 0, sizeof(*parser));
    }
}


bool ball_vision_parser_push(
    ball_vision_parser_t *parser,
    uint8_t byte,
    ball_vision_packet_t *packet_out
) {
    ball_vision_packet_t packet;
    if (parser == NULL || packet_out == NULL) {
        return false;
    }

    if (parser->index >= BALL_VISION_PACKET_SIZE) {
        parser->index = 0u;
    }
    if (parser->index == 0u) {
        if (byte == BALL_VISION_MAGIC_0) {
            parser->bytes[0] = byte;
            parser->index = 1u;
        }
        return false;
    }
    if (parser->index == 1u) {
        if (byte == BALL_VISION_MAGIC_1) {
            parser->bytes[1] = byte;
            parser->index = 2u;
        } else if (byte == BALL_VISION_MAGIC_0) {
            parser->bytes[0] = byte;
        } else {
            parser->index = 0u;
        }
        return false;
    }

    parser->bytes[parser->index++] = byte;
    if (parser->index < BALL_VISION_PACKET_SIZE) {
        return false;
    }
    decode_packet(parser->bytes, &packet);
    if (!ball_vision_packet_valid(&packet)) {
        parser_resync(parser);
        return false;
    }
    parser->index = 0u;
    *packet_out = packet;
    return true;
}
