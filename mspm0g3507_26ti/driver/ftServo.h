#ifndef FT_SERVO_H
#define FT_SERVO_H

#include <stdbool.h>
#include <stdint.h>

#define SERVO_UART_BAUD_RATE      (115200U)
#define SERVO_MOVE_FRAME_LENGTH   (13U)

/* The pitch/Y-axis servo in the 2025-E-Car project uses ID 2. */
#define SERVO_Y_ID                (2U)
#define SERVO_Y_POSITION_MIN      (0U)
#define SERVO_Y_POSITION_CENTER   (2048U)
#define SERVO_Y_POSITION_MAX      (4095U)
/* STS3250 uses little-endian words: 1000 is sent as E8 03. */
#define SERVO_Y_DEFAULT_SPEED     (1000U)

void Servo_MoveToPosition(uint8_t id, uint16_t position, uint16_t speed);

/* Call Servo_Y_Loop from the existing 200 Hz task for a 100 Hz refresh. */
void Servo_Y_Init(void);
void Servo_Y_InitAtAngle(float neutral_angle_deg, uint16_t speed);
void Servo_Y_Enable(bool enable);
void Servo_Y_SetTorque(bool enable);
/* These setters update the command target; Servo_Y_SendNow sends immediately. */
void Servo_Y_SetTarget(uint16_t position, uint16_t speed);
void Servo_Y_SetAngle(float angle_deg, uint16_t speed);
void Servo_Y_SendNow(void);
void Servo_Y_Loop(void);
uint16_t Servo_Y_GetTarget(void);
uint16_t Servo_Y_AngleToEncoder(float angle_deg);

#endif
