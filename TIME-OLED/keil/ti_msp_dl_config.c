/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gPWM_LEFTBackup;
DL_TimerG_backupConfig gPWM_RIGHTBackup;
DL_TimerA_backupConfig gTIMER_0Backup;
DL_UART_Main_backupConfig gUART_3Backup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_LEFT_init();
    SYSCFG_DL_PWM_RIGHT_init();
    SYSCFG_DL_TIMER_TICK_init();
    SYSCFG_DL_TIMER_0_init();
    SYSCFG_DL_UART_0_init();
    SYSCFG_DL_UART_1_init();
    SYSCFG_DL_UART_3_init();
    SYSCFG_DL_UART_2_init();
    SYSCFG_DL_SYSTICK_init();
    /* Ensure backup structures have no valid state */
	gPWM_LEFTBackup.backupRdy 	= false;
	gPWM_RIGHTBackup.backupRdy 	= false;
	gTIMER_0Backup.backupRdy 	= false;
	gUART_3Backup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(PWM_LEFT_INST, &gPWM_LEFTBackup);
	retStatus &= DL_TimerG_saveConfiguration(PWM_RIGHT_INST, &gPWM_RIGHTBackup);
	retStatus &= DL_TimerA_saveConfiguration(TIMER_0_INST, &gTIMER_0Backup);
	retStatus &= DL_UART_Main_saveConfiguration(UART_3_INST, &gUART_3Backup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(PWM_LEFT_INST, &gPWM_LEFTBackup, false);
	retStatus &= DL_TimerG_restoreConfiguration(PWM_RIGHT_INST, &gPWM_RIGHTBackup, false);
	retStatus &= DL_TimerA_restoreConfiguration(TIMER_0_INST, &gTIMER_0Backup, false);
	retStatus &= DL_UART_Main_restoreConfiguration(UART_3_INST, &gUART_3Backup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(PWM_LEFT_INST);
    DL_TimerG_reset(PWM_RIGHT_INST);
    DL_TimerG_reset(TIMER_TICK_INST);
    DL_TimerA_reset(TIMER_0_INST);
    DL_UART_Main_reset(UART_0_INST);
    DL_UART_Main_reset(UART_1_INST);
    DL_UART_Main_reset(UART_3_INST);
    DL_UART_Main_reset(UART_2_INST);


    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(PWM_LEFT_INST);
    DL_TimerG_enablePower(PWM_RIGHT_INST);
    DL_TimerG_enablePower(TIMER_TICK_INST);
    DL_TimerA_enablePower(TIMER_0_INST);
    DL_UART_Main_enablePower(UART_0_INST);
    DL_UART_Main_enablePower(UART_1_INST);
    DL_UART_Main_enablePower(UART_3_INST);
    DL_UART_Main_enablePower(UART_2_INST);

    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_LEFT_C0_IOMUX,GPIO_PWM_LEFT_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_LEFT_C0_PORT, GPIO_PWM_LEFT_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_LEFT_C1_IOMUX,GPIO_PWM_LEFT_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_LEFT_C1_PORT, GPIO_PWM_LEFT_C1_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_RIGHT_C0_IOMUX,GPIO_PWM_RIGHT_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_RIGHT_C0_PORT, GPIO_PWM_RIGHT_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_RIGHT_C1_IOMUX,GPIO_PWM_RIGHT_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_RIGHT_C1_PORT, GPIO_PWM_RIGHT_C1_PIN);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_0_IOMUX_TX, GPIO_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_0_IOMUX_RX, GPIO_UART_0_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_1_IOMUX_TX, GPIO_UART_1_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_1_IOMUX_RX, GPIO_UART_1_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_3_IOMUX_TX, GPIO_UART_3_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_3_IOMUX_RX, GPIO_UART_3_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_2_IOMUX_TX, GPIO_UART_2_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_2_IOMUX_RX, GPIO_UART_2_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalOutput(PORTB_LED_IOMUX);

    DL_GPIO_initDigitalOutput(PORTB_OLED_SCL_IOMUX);

    DL_GPIO_initDigitalOutput(PORTB_OLED_SDA_IOMUX);

    DL_GPIO_initDigitalInputFeatures(PORTB_KEY_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(PORTA_KEY_BSL_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInput(PORTA_L2_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_L1_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_M_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_R1_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_R2_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_E2B_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_E1A_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_E1B_IOMUX);

    DL_GPIO_initDigitalInput(PORTA_E2A_IOMUX);

    DL_GPIO_initDigitalOutput(PORTA_SCL_IOMUX);

    DL_GPIO_initDigitalOutputFeatures(PORTA_SDA_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_clearPins(PORTA_PORT, PORTA_SDA_PIN);
    DL_GPIO_setPins(PORTA_PORT, PORTA_SCL_PIN);
    DL_GPIO_enableOutput(PORTA_PORT, PORTA_SCL_PIN |
		PORTA_SDA_PIN);
    DL_GPIO_setLowerPinsPolarity(PORTA_PORT, DL_GPIO_PIN_15_EDGE_RISE |
		DL_GPIO_PIN_12_EDGE_RISE |
		DL_GPIO_PIN_13_EDGE_RISE |
		DL_GPIO_PIN_14_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(PORTA_PORT, PORTA_E2B_PIN |
		PORTA_E1A_PIN |
		PORTA_E1B_PIN |
		PORTA_E2A_PIN);
    DL_GPIO_enableInterrupt(PORTA_PORT, PORTA_E2B_PIN |
		PORTA_E1A_PIN |
		PORTA_E1B_PIN |
		PORTA_E2A_PIN);
    DL_GPIO_clearPins(PORTB_PORT, PORTB_LED_PIN |
		PORTB_OLED_SCL_PIN |
		PORTB_OLED_SDA_PIN);
    DL_GPIO_enableOutput(PORTB_PORT, PORTB_LED_PIN |
		PORTB_OLED_SCL_PIN |
		PORTB_OLED_SDA_PIN);

}



SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* INT_GROUP1 Priority */
    NVIC_SetPriority(GPIOA_INT_IRQn, 0);

}


/*
 * Timer clock configuration to be sourced by  / 8 (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 4000000 Hz / (8 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gPWM_LEFTClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gPWM_LEFTConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 1000,
    .isTimerWithFourCC = true,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_LEFT_init(void) {

    DL_TimerA_setClockConfig(
        PWM_LEFT_INST, (DL_TimerA_ClockConfig *) &gPWM_LEFTClockConfig);

    DL_TimerA_initPWMMode(
        PWM_LEFT_INST, (DL_TimerA_PWMConfig *) &gPWM_LEFTConfig);

    DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, 1000, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareOutCtl(PWM_LEFT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_LEFT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, 1000, DL_TIMER_CC_1_INDEX);
    DL_TimerA_setCaptureCompareOutCtl(PWM_LEFT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_1_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_LEFT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_1_INDEX);

    DL_TimerA_enableClock(PWM_LEFT_INST);


    
    DL_TimerA_setCCPDirection(PWM_LEFT_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );

}
/*
 * Timer clock configuration to be sourced by  / 8 (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 4000000 Hz / (8 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gPWM_RIGHTClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gPWM_RIGHTConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 1000,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_RIGHT_init(void) {

    DL_TimerG_setClockConfig(
        PWM_RIGHT_INST, (DL_TimerG_ClockConfig *) &gPWM_RIGHTClockConfig);

    DL_TimerG_initPWMMode(
        PWM_RIGHT_INST, (DL_TimerG_PWMConfig *) &gPWM_RIGHTConfig);

    DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, 1000, DL_TIMER_CC_0_INDEX);
    DL_TimerG_setCaptureCompareOutCtl(PWM_RIGHT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_RIGHT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_0_INDEX);

    DL_TimerG_setCaptureCompareValue(PWM_RIGHT_INST, 1000, DL_TIMER_CC_1_INDEX);
    DL_TimerG_setCaptureCompareOutCtl(PWM_RIGHT_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_RIGHT_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_enableClock(PWM_RIGHT_INST);


    
    DL_TimerG_setCCPDirection(PWM_RIGHT_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );

}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000 Hz = 4000000 Hz / (8 * (99 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_TICKClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 99U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_TICK_INST_LOAD_VALUE = (10ms * 40000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_TICKTimerConfig = {
    .period     = TIMER_TICK_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_TICK_init(void) {

    DL_TimerG_setClockConfig(TIMER_TICK_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_TICKClockConfig);

    DL_TimerG_initTimerMode(TIMER_TICK_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_TICKTimerConfig);
    DL_TimerG_enableInterrupt(TIMER_TICK_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
	NVIC_SetPriority(TIMER_TICK_INST_INT_IRQN, 0);
    DL_TimerG_enableClock(TIMER_TICK_INST);




}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000 Hz = 4000000 Hz / (8 * (99 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 99U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_0_INST_LOAD_VALUE = (100 ms * 40000 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_0TimerConfig = {
    .period     = TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_0_init(void) {

    DL_TimerA_setClockConfig(TIMER_0_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_0ClockConfig);

    DL_TimerA_initTimerMode(TIMER_0_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_0TimerConfig);
    DL_TimerA_enableInterrupt(TIMER_0_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
	NVIC_SetPriority(TIMER_0_INST_INT_IRQN, 3);
    DL_TimerA_enableClock(TIMER_0_INST);




}



static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(UART_0_INST, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);

    DL_UART_Main_init(UART_0_INST, (DL_UART_Main_Config *) &gUART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_0_INST, UART_0_IBRD_32_MHZ_115200_BAUD, UART_0_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(UART_0_INST);
}

static const DL_UART_Main_ClockConfig gUART_1ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_1Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_1_init(void)
{
    DL_UART_Main_setClockConfig(UART_1_INST, (DL_UART_Main_ClockConfig *) &gUART_1ClockConfig);

    DL_UART_Main_init(UART_1_INST, (DL_UART_Main_Config *) &gUART_1Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_1_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_1_INST, UART_1_IBRD_32_MHZ_115200_BAUD, UART_1_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_1_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);
    /* Setting the Interrupt Priority */
    NVIC_SetPriority(UART_1_INST_INT_IRQN, 0);


    DL_UART_Main_enable(UART_1_INST);
}

static const DL_UART_Main_ClockConfig gUART_3ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_3Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_3_init(void)
{
    DL_UART_Main_setClockConfig(UART_3_INST, (DL_UART_Main_ClockConfig *) &gUART_3ClockConfig);

    DL_UART_Main_init(UART_3_INST, (DL_UART_Main_Config *) &gUART_3Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_3_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_3_INST, UART_3_IBRD_32_MHZ_115200_BAUD, UART_3_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_3_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);
    /* Setting the Interrupt Priority */
    NVIC_SetPriority(UART_3_INST_INT_IRQN, 0);


    DL_UART_Main_enable(UART_3_INST);
}

static const DL_UART_Main_ClockConfig gUART_2ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_2Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_2_init(void)
{
    DL_UART_Main_setClockConfig(UART_2_INST, (DL_UART_Main_ClockConfig *) &gUART_2ClockConfig);

    DL_UART_Main_init(UART_2_INST, (DL_UART_Main_Config *) &gUART_2Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_2_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_2_INST, UART_2_IBRD_32_MHZ_115200_BAUD, UART_2_FBRD_32_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_2_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);
    /* Setting the Interrupt Priority */
    NVIC_SetPriority(UART_2_INST_INT_IRQN, 0);


    DL_UART_Main_enable(UART_2_INST);
}

SYSCONFIG_WEAK void SYSCFG_DL_SYSTICK_init(void)
{
    /*
     * Initializes the SysTick period to 1.00 ms,
     * enables the interrupt, and starts the SysTick Timer
     */
    DL_SYSTICK_config(32000);
}

