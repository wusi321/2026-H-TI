#include "ti_msp_dl_config.h"
#include "ftServo.h"
#include "nuart.h"
#include "system.h"

#define SERVO_FRAME_HEADER        (0xFFU)
#define SERVO_FRAME_DATA_LENGTH   (0x09U)
#define SERVO_INSTRUCTION_WRITE   (0x03U)
#define SERVO_TORQUE_ENABLE_ADDR  (0x28U)
#define SERVO_ACCELERATION_ADDR   (0x29U)
#define SERVO_GOAL_POSITION_ADDR  (0x2AU)
#define SERVO_FAST_ACCELERATION   (0U)
#define SERVO_WRITE_BYTE_LENGTH   (8U)
#define SERVO_LOOP_DIVIDER          (2U)
#define SERVO_TORQUE_ENABLE_RETRIES (3U)
#define SERVO_COMMAND_REPLY_GAP_MS  (10U)

#if UART_3_BAUD_RATE != SERVO_UART_BAUD_RATE
#error "UART3 must match the 115200-baud Maix servo link"
#endif

typedef struct {
    uint16_t position;
    uint16_t speed;
    uint8_t loop_count;
    uint8_t torque_enable_retries;
    bool enabled;
} ServoYState;

static ServoYState g_servo_y = {
    SERVO_Y_POSITION_CENTER,
    SERVO_Y_DEFAULT_SPEED,
    0U,
    0U,
    true
};

static uint8_t Servo_CalculateChecksum(const uint8_t *frame, uint8_t length)
{
    uint8_t checksum = 0U;
    uint8_t i;

    for (i = 2U; i < (length - 1U); i++) {
        checksum = (uint8_t)(checksum + frame[i]);
    }

    return (uint8_t)(~checksum);
}

static void Servo_WriteByte(uint8_t id, uint8_t address, uint8_t value)
{
    uint8_t frame[SERVO_WRITE_BYTE_LENGTH];

    frame[0] = SERVO_FRAME_HEADER;
    frame[1] = SERVO_FRAME_HEADER;
    frame[2] = id;
    frame[3] = 0x04U;
    frame[4] = SERVO_INSTRUCTION_WRITE;
    frame[5] = address;
    frame[6] = value;
    frame[7] = Servo_CalculateChecksum(frame, SERVO_WRITE_BYTE_LENGTH);

    UART_SendBytes(UART_3_INST, frame, SERVO_WRITE_BYTE_LENGTH);
}

static uint16_t Servo_Y_ClampPosition(uint16_t position)
{
    if (position > SERVO_Y_POSITION_MAX) {
        return SERVO_Y_POSITION_MAX;
    }

    return position;
}

void Servo_MoveToPosition(uint8_t id, uint16_t position, uint16_t speed)
{
    uint8_t frame[SERVO_MOVE_FRAME_LENGTH];

    position = Servo_Y_ClampPosition(position);
    frame[0] = SERVO_FRAME_HEADER;
    frame[1] = SERVO_FRAME_HEADER;
    frame[2] = id;
    frame[3] = SERVO_FRAME_DATA_LENGTH;
    frame[4] = SERVO_INSTRUCTION_WRITE;
    frame[5] = SERVO_GOAL_POSITION_ADDR;
    frame[6] = (uint8_t)(position & 0xFFU);
    frame[7] = (uint8_t)((position >> 8U) & 0xFFU);
    frame[8] = 0x00U;
    frame[9] = 0x00U;
    frame[10] = (uint8_t)(speed & 0xFFU);
    frame[11] = (uint8_t)((speed >> 8U) & 0xFFU);
    frame[12] = Servo_CalculateChecksum(frame, SERVO_MOVE_FRAME_LENGTH);

    UART_SendBytes(UART_3_INST, frame, SERVO_MOVE_FRAME_LENGTH);
}

uint16_t Servo_Y_AngleToEncoder(float angle_deg)
{
    float encoder;

    if (angle_deg <= -180.0f) {
        return SERVO_Y_POSITION_MIN;
    }
    if (angle_deg >= 180.0f) {
        return SERVO_Y_POSITION_MAX;
    }

    encoder = ((angle_deg + 180.0f) *
               (float)SERVO_Y_POSITION_MAX) / 360.0f;
    return (uint16_t)(encoder + 0.5f);
}

void Servo_Y_InitAtAngle(float neutral_angle_deg, uint16_t speed)
{
    g_servo_y.position = Servo_Y_AngleToEncoder(neutral_angle_deg);
    g_servo_y.speed = speed;
    g_servo_y.loop_count = 0U;
    g_servo_y.torque_enable_retries = SERVO_TORQUE_ENABLE_RETRIES;
    g_servo_y.enabled = true;

    /* The controller owns the angle slew, so keep the servo's internal
     * acceleration limiter from adding an unknown extra delay. */
    Servo_WriteByte(SERVO_Y_ID, SERVO_ACCELERATION_ADDR,
                    SERVO_FAST_ACCELERATION);
    delay_ms(SERVO_COMMAND_REPLY_GAP_MS);

    /* Load the safe target before torque is enabled. */
    Servo_Y_SendNow();
    delay_ms(SERVO_COMMAND_REPLY_GAP_MS);
    Servo_Y_SetTorque(true);
    g_servo_y.torque_enable_retries--;
}

void Servo_Y_Init(void)
{
    Servo_Y_InitAtAngle(0.0f, SERVO_Y_DEFAULT_SPEED);
}

void Servo_Y_Enable(bool enable)
{
    g_servo_y.enabled = enable;
    g_servo_y.loop_count = 0U;
    if (enable) {
        g_servo_y.torque_enable_retries = SERVO_TORQUE_ENABLE_RETRIES;
        Servo_Y_SendNow();
    } else {
        g_servo_y.torque_enable_retries = 0U;
        Servo_Y_SetTorque(false);
    }
}

void Servo_Y_SetTorque(bool enable)
{
    Servo_WriteByte(SERVO_Y_ID, SERVO_TORQUE_ENABLE_ADDR,
                    enable ? 1U : 0U);
}

void Servo_Y_SetTarget(uint16_t position, uint16_t speed)
{
    g_servo_y.position = Servo_Y_ClampPosition(position);
    g_servo_y.speed = speed;
}

void Servo_Y_SetAngle(float angle_deg, uint16_t speed)
{
    Servo_Y_SetTarget(Servo_Y_AngleToEncoder(angle_deg), speed);
}

void Servo_Y_SendNow(void)
{
    if (!g_servo_y.enabled) {
        return;
    }

    Servo_MoveToPosition(SERVO_Y_ID, g_servo_y.position, g_servo_y.speed);
}

void Servo_Y_Loop(void)
{
    if (!g_servo_y.enabled) {
        return;
    }

    g_servo_y.loop_count++;
    if (g_servo_y.loop_count < SERVO_LOOP_DIVIDER) {
        return;
    }

    g_servo_y.loop_count = 0U;

    if (g_servo_y.torque_enable_retries > 0U) {
        Servo_Y_SetTorque(true);
        g_servo_y.torque_enable_retries--;
        return;
    }

    Servo_Y_SendNow();
}

uint16_t Servo_Y_GetTarget(void)
{
    return g_servo_y.position;
}
