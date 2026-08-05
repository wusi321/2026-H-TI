/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_LEFT */
#define PWM_LEFT_INST                                                      TIMA0
#define PWM_LEFT_INST_IRQHandler                                TIMA0_IRQHandler
#define PWM_LEFT_INST_INT_IRQN                                  (TIMA0_INT_IRQn)
#define PWM_LEFT_INST_CLK_FREQ                                           4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_LEFT_C0_PORT                                              GPIOB
#define GPIO_PWM_LEFT_C0_PIN                                      DL_GPIO_PIN_14
#define GPIO_PWM_LEFT_C0_IOMUX                                   (IOMUX_PINCM31)
#define GPIO_PWM_LEFT_C0_IOMUX_FUNC                  IOMUX_PINCM31_PF_TIMA0_CCP0
#define GPIO_PWM_LEFT_C0_IDX                                 DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_LEFT_C1_PORT                                              GPIOB
#define GPIO_PWM_LEFT_C1_PIN                                      DL_GPIO_PIN_12
#define GPIO_PWM_LEFT_C1_IOMUX                                   (IOMUX_PINCM29)
#define GPIO_PWM_LEFT_C1_IOMUX_FUNC                  IOMUX_PINCM29_PF_TIMA0_CCP1
#define GPIO_PWM_LEFT_C1_IDX                                 DL_TIMER_CC_1_INDEX

/* Defines for PWM_RIGHT */
#define PWM_RIGHT_INST                                                     TIMG7
#define PWM_RIGHT_INST_IRQHandler                               TIMG7_IRQHandler
#define PWM_RIGHT_INST_INT_IRQN                                 (TIMG7_INT_IRQn)
#define PWM_RIGHT_INST_CLK_FREQ                                          4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_RIGHT_C0_PORT                                             GPIOA
#define GPIO_PWM_RIGHT_C0_PIN                                     DL_GPIO_PIN_17
#define GPIO_PWM_RIGHT_C0_IOMUX                                  (IOMUX_PINCM39)
#define GPIO_PWM_RIGHT_C0_IOMUX_FUNC                 IOMUX_PINCM39_PF_TIMG7_CCP0
#define GPIO_PWM_RIGHT_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_RIGHT_C1_PORT                                             GPIOA
#define GPIO_PWM_RIGHT_C1_PIN                                      DL_GPIO_PIN_7
#define GPIO_PWM_RIGHT_C1_IOMUX                                  (IOMUX_PINCM14)
#define GPIO_PWM_RIGHT_C1_IOMUX_FUNC                 IOMUX_PINCM14_PF_TIMG7_CCP1
#define GPIO_PWM_RIGHT_C1_IDX                                DL_TIMER_CC_1_INDEX



/* Defines for TIMER_TICK */
#define TIMER_TICK_INST                                                  (TIMG0)
#define TIMER_TICK_INST_IRQHandler                              TIMG0_IRQHandler
#define TIMER_TICK_INST_INT_IRQN                                (TIMG0_INT_IRQn)
#define TIMER_TICK_INST_LOAD_VALUE                                        (399U)
/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA1)
#define TIMER_0_INST_IRQHandler                                 TIMA1_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA1_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                          (3999U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_9
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_8
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM20)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM19)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM20_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM19_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_1_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_3 */
#define UART_3_INST                                                        UART3
#define UART_3_INST_IRQHandler                                  UART3_IRQHandler
#define UART_3_INST_INT_IRQN                                      UART3_INT_IRQn
#define GPIO_UART_3_RX_PORT                                                GPIOB
#define GPIO_UART_3_TX_PORT                                                GPIOB
#define GPIO_UART_3_RX_PIN                                         DL_GPIO_PIN_3
#define GPIO_UART_3_TX_PIN                                         DL_GPIO_PIN_2
#define GPIO_UART_3_IOMUX_RX                                     (IOMUX_PINCM16)
#define GPIO_UART_3_IOMUX_TX                                     (IOMUX_PINCM15)
#define GPIO_UART_3_IOMUX_RX_FUNC                      IOMUX_PINCM16_PF_UART3_RX
#define GPIO_UART_3_IOMUX_TX_FUNC                      IOMUX_PINCM15_PF_UART3_TX
#define UART_3_BAUD_RATE                                                (115200)
#define UART_3_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_3_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_2 */
#define UART_2_INST                                                        UART2
#define UART_2_INST_IRQHandler                                  UART2_IRQHandler
#define UART_2_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_2_RX_PORT                                                GPIOA
#define GPIO_UART_2_TX_PORT                                                GPIOA
#define GPIO_UART_2_RX_PIN                                        DL_GPIO_PIN_22
#define GPIO_UART_2_TX_PIN                                        DL_GPIO_PIN_21
#define GPIO_UART_2_IOMUX_RX                                     (IOMUX_PINCM47)
#define GPIO_UART_2_IOMUX_TX                                     (IOMUX_PINCM46)
#define GPIO_UART_2_IOMUX_RX_FUNC                      IOMUX_PINCM47_PF_UART2_RX
#define GPIO_UART_2_IOMUX_TX_FUNC                      IOMUX_PINCM46_PF_UART2_TX
#define UART_2_BAUD_RATE                                                (115200)
#define UART_2_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_2_FBRD_32_MHZ_115200_BAUD                                      (23)





/* Port definition for Pin Group PORTB */
#define PORTB_PORT                                                       (GPIOB)

/* Defines for LED: GPIOB.22 with pinCMx 50 on package pin 21 */
#define PORTB_LED_PIN                                           (DL_GPIO_PIN_22)
#define PORTB_LED_IOMUX                                          (IOMUX_PINCM50)
/* Defines for OLED_SCL: GPIOB.9 with pinCMx 26 on package pin 61 */
#define PORTB_OLED_SCL_PIN                                       (DL_GPIO_PIN_9)
#define PORTB_OLED_SCL_IOMUX                                     (IOMUX_PINCM26)
/* Defines for OLED_SDA: GPIOB.8 with pinCMx 25 on package pin 60 */
#define PORTB_OLED_SDA_PIN                                       (DL_GPIO_PIN_8)
#define PORTB_OLED_SDA_IOMUX                                     (IOMUX_PINCM25)
/* Defines for KEY: GPIOB.21 with pinCMx 49 on package pin 20 */
#define PORTB_KEY_PIN                                           (DL_GPIO_PIN_21)
#define PORTB_KEY_IOMUX                                          (IOMUX_PINCM49)
/* Port definition for Pin Group PORTA */
#define PORTA_PORT                                                       (GPIOA)

/* Defines for KEY_BSL: GPIOA.18 with pinCMx 40 on package pin 11 */
#define PORTA_KEY_BSL_PIN                                       (DL_GPIO_PIN_18)
#define PORTA_KEY_BSL_IOMUX                                      (IOMUX_PINCM40)
/* Defines for L2: GPIOA.31 with pinCMx 6 on package pin 39 */
#define PORTA_L2_PIN                                            (DL_GPIO_PIN_31)
#define PORTA_L2_IOMUX                                            (IOMUX_PINCM6)
/* Defines for L1: GPIOA.28 with pinCMx 3 on package pin 35 */
#define PORTA_L1_PIN                                            (DL_GPIO_PIN_28)
#define PORTA_L1_IOMUX                                            (IOMUX_PINCM3)
/* Defines for M: GPIOA.1 with pinCMx 2 on package pin 34 */
#define PORTA_M_PIN                                              (DL_GPIO_PIN_1)
#define PORTA_M_IOMUX                                             (IOMUX_PINCM2)
/* Defines for R1: GPIOA.0 with pinCMx 1 on package pin 33 */
#define PORTA_R1_PIN                                             (DL_GPIO_PIN_0)
#define PORTA_R1_IOMUX                                            (IOMUX_PINCM1)
/* Defines for R2: GPIOA.25 with pinCMx 55 on package pin 26 */
#define PORTA_R2_PIN                                            (DL_GPIO_PIN_25)
#define PORTA_R2_IOMUX                                           (IOMUX_PINCM55)
/* Defines for E2B: GPIOA.15 with pinCMx 37 on package pin 8 */
// pins affected by this interrupt request:["E2B","E1A","E1B","E2A"]
#define PORTA_INT_IRQN                                          (GPIOA_INT_IRQn)
#define PORTA_INT_IIDX                          (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define PORTA_E2B_IIDX                                      (DL_GPIO_IIDX_DIO15)
#define PORTA_E2B_PIN                                           (DL_GPIO_PIN_15)
#define PORTA_E2B_IOMUX                                          (IOMUX_PINCM37)
/* Defines for E1A: GPIOA.12 with pinCMx 34 on package pin 5 */
#define PORTA_E1A_IIDX                                      (DL_GPIO_IIDX_DIO12)
#define PORTA_E1A_PIN                                           (DL_GPIO_PIN_12)
#define PORTA_E1A_IOMUX                                          (IOMUX_PINCM34)
/* Defines for E1B: GPIOA.13 with pinCMx 35 on package pin 6 */
#define PORTA_E1B_IIDX                                      (DL_GPIO_IIDX_DIO13)
#define PORTA_E1B_PIN                                           (DL_GPIO_PIN_13)
#define PORTA_E1B_IOMUX                                          (IOMUX_PINCM35)
/* Defines for E2A: GPIOA.14 with pinCMx 36 on package pin 7 */
#define PORTA_E2A_IIDX                                      (DL_GPIO_IIDX_DIO14)
#define PORTA_E2A_PIN                                           (DL_GPIO_PIN_14)
#define PORTA_E2A_IOMUX                                          (IOMUX_PINCM36)
/* Defines for SCL: GPIOA.29 with pinCMx 4 on package pin 36 */
#define PORTA_SCL_PIN                                           (DL_GPIO_PIN_29)
#define PORTA_SCL_IOMUX                                           (IOMUX_PINCM4)
/* Defines for SDA: GPIOA.30 with pinCMx 5 on package pin 37 */
#define PORTA_SDA_PIN                                           (DL_GPIO_PIN_30)
#define PORTA_SDA_IOMUX                                           (IOMUX_PINCM5)



/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_LEFT_init(void);
void SYSCFG_DL_PWM_RIGHT_init(void);
void SYSCFG_DL_TIMER_TICK_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_UART_3_init(void);
void SYSCFG_DL_UART_2_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
