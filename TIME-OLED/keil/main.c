#include "ti_msp_dl_config.h"
#include "oled.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

uint8_t current_task = 1U;

extern const unsigned char F6x8[][6];

#define STOPWATCH_RUN_PORT            PORTA_PORT
#define STOPWATCH_RUN_PIN             PORTA_E2B_PIN
#define STOPWATCH_RUN_IOMUX           PORTA_E2B_IOMUX

#define STOPWATCH_RESET_KEY_PORT      PORTB_PORT
#define STOPWATCH_RESET_KEY_PIN       PORTB_KEY_PIN

#define STOPWATCH_DEBOUNCE_MS         (35U)
#define STOPWATCH_DISPLAY_PERIOD_MS   (50U)
#define STOPWATCH_LED_PERIOD_MS       (500U)

typedef enum {
    STOPWATCH_PAUSED = 0,
    STOPWATCH_RUNNING
} stopwatch_state_t;

static volatile uint32_t g_system_ms;
static volatile uint32_t g_elapsed_ms;
static volatile bool g_tick_pending;
static volatile stopwatch_state_t g_stopwatch_state;

static uint32_t g_last_display_ms;
static uint32_t g_last_led_ms;
static uint32_t g_key_changed_ms;
static bool g_last_key_pressed;
static bool g_stable_key_pressed;

static void stopwatch_gpio_prepare(void);
static bool take_1ms_tick(uint32_t *now_ms);
static uint32_t stopwatch_elapsed_ms(void);
static uint32_t stopwatch_millis(void);
static stopwatch_state_t stopwatch_state(void);
static void stopwatch_reset(void);
static bool run_pin_is_high(void);
static bool reset_key_is_pressed(void);
static void run_pin_scan(void);
static void reset_key_scan(uint32_t now_ms);
static uint16_t oled_expand_byte_2x(uint8_t value);
static void oled_draw_big_char(uint8_t x, uint8_t page, char ch);
static void oled_draw_big_text(uint8_t x, uint8_t page, const char *text);
static void oled_draw_static_frame(void);
static void oled_update_stopwatch(void);
static const char *stopwatch_state_text(stopwatch_state_t state);
static void format_elapsed_time(uint32_t ms, char *buffer, uint32_t buffer_len);

int main(void)
{
    SYSCFG_DL_init();
    stopwatch_gpio_prepare();

    __disable_irq();
    g_system_ms = 0U;
    g_elapsed_ms = 0U;
    g_tick_pending = false;
    g_stopwatch_state = STOPWATCH_PAUSED;
    __enable_irq();

    oled_init();

    g_last_key_pressed = reset_key_is_pressed();
    g_stable_key_pressed = g_last_key_pressed;
    g_key_changed_ms = stopwatch_millis();
    g_last_display_ms = g_key_changed_ms;
    g_last_led_ms = g_key_changed_ms;

    run_pin_scan();
    oled_draw_static_frame();
    oled_update_stopwatch();

    while (1) {
        uint32_t now;

        if (take_1ms_tick(&now)) {
            run_pin_scan();
            reset_key_scan(now);

            if ((now - g_last_display_ms) >= STOPWATCH_DISPLAY_PERIOD_MS) {
                g_last_display_ms = now;
                oled_update_stopwatch();
            }

            if ((now - g_last_led_ms) >= STOPWATCH_LED_PERIOD_MS) {
                g_last_led_ms = now;
                DL_GPIO_togglePins(PORTB_PORT, PORTB_LED_PIN);
            }
        }

        __WFI();
    }
}

void SysTick_Handler(void)
{
    g_system_ms++;
    g_tick_pending = true;

    if (g_stopwatch_state == STOPWATCH_RUNNING) {
        g_elapsed_ms++;
    }
}

static void stopwatch_gpio_prepare(void)
{
    NVIC_DisableIRQ(GPIOA_INT_IRQn);
    NVIC_ClearPendingIRQ(GPIOA_INT_IRQn);
    DL_GPIO_disableInterrupt(PORTA_PORT,
        PORTA_E2B_PIN | PORTA_E2A_PIN | PORTA_E1A_PIN | PORTA_E1B_PIN);
    DL_GPIO_clearInterruptStatus(PORTA_PORT,
        PORTA_E2B_PIN | PORTA_E2A_PIN | PORTA_E1A_PIN | PORTA_E1B_PIN);

    DL_GPIO_initDigitalInputFeatures(STOPWATCH_RUN_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static bool take_1ms_tick(uint32_t *now_ms)
{
    bool ticked;

    __disable_irq();
    ticked = g_tick_pending;
    if (ticked) {
        g_tick_pending = false;
        *now_ms = g_system_ms;
    }
    __enable_irq();

    return ticked;
}

static uint32_t stopwatch_elapsed_ms(void)
{
    uint32_t elapsed;

    __disable_irq();
    elapsed = g_elapsed_ms;
    __enable_irq();

    return elapsed;
}

static uint32_t stopwatch_millis(void)
{
    uint32_t now;

    __disable_irq();
    now = g_system_ms;
    __enable_irq();

    return now;
}

static stopwatch_state_t stopwatch_state(void)
{
    stopwatch_state_t state;

    __disable_irq();
    state = g_stopwatch_state;
    __enable_irq();

    return state;
}

static void stopwatch_reset(void)
{
    __disable_irq();
    g_elapsed_ms = 0U;
    __enable_irq();

    oled_update_stopwatch();
}

static bool run_pin_is_high(void)
{
    return (DL_GPIO_readPins(STOPWATCH_RUN_PORT,
        STOPWATCH_RUN_PIN) & STOPWATCH_RUN_PIN) != 0U;
}

static bool reset_key_is_pressed(void)
{
    return (DL_GPIO_readPins(STOPWATCH_RESET_KEY_PORT,
        STOPWATCH_RESET_KEY_PIN) & STOPWATCH_RESET_KEY_PIN) == 0U;
}

static void run_pin_scan(void)
{
    stopwatch_state_t next_state = run_pin_is_high() ?
        STOPWATCH_RUNNING : STOPWATCH_PAUSED;

    __disable_irq();
    g_stopwatch_state = next_state;
    __enable_irq();
}

static void reset_key_scan(uint32_t now_ms)
{
    bool pressed = reset_key_is_pressed();

    if (pressed != g_last_key_pressed) {
        g_last_key_pressed = pressed;
        g_key_changed_ms = now_ms;
    }

    if (((now_ms - g_key_changed_ms) >= STOPWATCH_DEBOUNCE_MS) &&
        (pressed != g_stable_key_pressed)) {
        g_stable_key_pressed = pressed;

        if (g_stable_key_pressed) {
            stopwatch_reset();
        }
    }
}

static uint16_t oled_expand_byte_2x(uint8_t value)
{
    uint16_t expanded = 0U;

    for (uint8_t bit = 0U; bit < 8U; bit++) {
        if ((value & (1U << bit)) != 0U) {
            expanded |= (uint16_t) (3U << (bit * 2U));
        }
    }

    return expanded;
}

static void oled_draw_big_char(uint8_t x, uint8_t page, char ch)
{
    uint8_t index = 0U;

    if ((ch >= ' ') && (ch <= '~')) {
        index = (uint8_t) (ch - ' ');
    }

    OLED_Set_Pos(x, page);
    for (uint8_t col = 0U; col < 6U; col++) {
        uint16_t expanded = oled_expand_byte_2x(F6x8[index][col]);
        OLED_WrDat((uint8_t) (expanded & 0xFFU));
        OLED_WrDat((uint8_t) (expanded & 0xFFU));
    }

    OLED_Set_Pos(x, (uint8_t) (page + 1U));
    for (uint8_t col = 0U; col < 6U; col++) {
        uint16_t expanded = oled_expand_byte_2x(F6x8[index][col]);
        OLED_WrDat((uint8_t) ((expanded >> 8U) & 0xFFU));
        OLED_WrDat((uint8_t) ((expanded >> 8U) & 0xFFU));
    }
}

static void oled_draw_big_text(uint8_t x, uint8_t page, const char *text)
{
    while ((*text != '\0') && (x <= 116U)) {
        oled_draw_big_char(x, page, *text);
        x = (uint8_t) (x + 12U);
        text++;
    }
}

static void oled_draw_static_frame(void)
{
    OLED_CLS();
    oled_draw_big_text(40U, 0U, "TIME");
    oled_draw_big_text(34U, 4U, "STATE");
}

static void oled_update_stopwatch(void)
{
    char time_text[12];
    stopwatch_state_t state = stopwatch_state();

    format_elapsed_time(stopwatch_elapsed_ms(), time_text,
        (uint32_t) sizeof(time_text));

    oled_draw_big_text(4U, 2U, time_text);
    oled_draw_big_text(34U, 6U, stopwatch_state_text(state));
}

static const char *stopwatch_state_text(stopwatch_state_t state)
{
    return (state == STOPWATCH_RUNNING) ? "RUN  " : "PAUSE";
}

static void format_elapsed_time(uint32_t ms, char *buffer, uint32_t buffer_len)
{
    uint32_t minutes = ms / 60000U;
    uint32_t seconds = (ms / 1000U) % 60U;
    uint32_t milliseconds = ms % 1000U;

    if (minutes > 99U) {
        minutes = 99U;
        seconds = 59U;
        milliseconds = 999U;
    }

    snprintf(buffer, buffer_len, "%02lu:%02lu.%03lu",
        (unsigned long) minutes,
        (unsigned long) seconds,
        (unsigned long) milliseconds);
}
