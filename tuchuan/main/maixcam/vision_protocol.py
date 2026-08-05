import struct


MAGIC = b"\xAA\x55"
VERSION = 1
FLAG_VALID = 0x01
FLAG_MEASURED = 0x02
FLAG_TRACKED = 0x04
FLAG_MASK = FLAG_VALID | FLAG_MEASURED | FLAG_TRACKED
INVALID_POSITION_MM = 32767

_BODY = struct.Struct("<2sBBHIhhHH")
_CRC = struct.Struct("<H")
PACKET_SIZE = _BODY.size + _CRC.size


def crc16_ccitt(data, initial=0xFFFF):
    crc = int(initial) & 0xFFFF
    for byte in data:
        crc ^= int(byte) << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def _int16(value):
    return max(-32768, min(32767, int(round(value))))


def encode_measurement(
    seq,
    timestamp_ms,
    state,
    measured,
    processing_us=0,
    force_invalid=False,
):
    valid = bool(state["valid"]) and not force_invalid
    flags = 0
    if valid:
        flags |= FLAG_VALID
    if valid and measured:
        flags |= FLAG_MEASURED
    if valid and state["initialized"] and not measured:
        flags |= FLAG_TRACKED

    if valid:
        x_mm = _int16(float(state["x_cm"]) * 10.0)
        velocity_mm_s = _int16(float(state["v_cm_s"]) * 10.0)
    else:
        x_mm = INVALID_POSITION_MM
        velocity_mm_s = 0

    confidence = 0 if force_invalid else max(
        0,
        min(1000, int(round(float(state["confidence"]) * 1000.0))),
    )
    body = _BODY.pack(
        MAGIC,
        VERSION,
        flags,
        int(seq) & 0xFFFF,
        int(timestamp_ms) & 0xFFFFFFFF,
        x_mm,
        velocity_mm_s,
        confidence,
        max(0, min(65535, int(processing_us))),
    )
    return body + _CRC.pack(crc16_ccitt(body))


def decode_measurement(packet):
    if len(packet) != PACKET_SIZE:
        raise ValueError("invalid packet size")
    body = packet[:-_CRC.size]
    expected_crc = _CRC.unpack(packet[-_CRC.size:])[0]
    if crc16_ccitt(body) != expected_crc:
        raise ValueError("CRC mismatch")
    magic, version, flags, seq, timestamp_ms, x_mm, velocity_mm_s, confidence, processing_us = _BODY.unpack(body)
    if magic != MAGIC or version != VERSION:
        raise ValueError("unsupported packet header")
    if flags & ~FLAG_MASK:
        raise ValueError("unsupported packet flags")
    valid = bool(flags & FLAG_VALID)
    measured = bool(flags & FLAG_MEASURED)
    tracked = bool(flags & FLAG_TRACKED)
    if confidence > 1000:
        raise ValueError("invalid packet confidence")
    if not valid and (measured or tracked or x_mm != INVALID_POSITION_MM or velocity_mm_s != 0):
        raise ValueError("invalid packet state")
    if measured and tracked:
        raise ValueError("conflicting packet state")
    return {
        "flags": flags,
        "valid": valid,
        "measured": measured,
        "tracked": tracked,
        "seq": seq,
        "timestamp_ms": timestamp_ms,
        "x_cm": x_mm / 10.0 if valid else None,
        "v_cm_s": velocity_mm_s / 10.0 if valid else None,
        "confidence": confidence / 1000.0,
        "processing_us": processing_us,
    }
