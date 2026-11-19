#include "host_bus.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

/*
 * Weekend Alpha Rev A.2 bus bridge
 * --------------------------------
 * Physical transport is deliberately tiny:
 *
 *   C64 write $DF00 -> one outgoing byte
 *   C64 read  $DF00 -> one status byte
 *
 * SN74LVC574A captures D0..D7 at the rising edge of WRITE_N. The same C64
 * write clears the hardware READY flip-flop immediately, before ESP32 code is
 * involved. GPIO12 sees that rising edge through the documented 10k/20k
 * divider. ESP32 copies the stable latch value and only then pulses GPIO39
 * low (open-drain) to preset READY back to 1.
 *
 * Because the C64 waits for READY before every byte, ISR latency cannot cause
 * a second byte to overwrite the latch.
 */

static const gpio_num_t byte_pins[8] = {
    FB_PIN_BUS_D0, FB_PIN_BUS_D1, FB_PIN_BUS_D2, FB_PIN_BUS_D3,
    FB_PIN_BUS_D4, FB_PIN_BUS_D5, FB_PIN_BUS_D6, FB_PIN_BUS_D7
};

/* Status bits 1..7. READY (bit 0) is generated entirely in hardware. */
static const gpio_num_t status_pins[7] = {
    FB_PIN_STATUS_BUSY,
    FB_PIN_STATUS_ERROR,
    FB_PIN_STATUS_VIDEO_LOCK,
    FB_PIN_STATUS_AUDIO,
    FB_PIN_STATUS_R5,
    FB_PIN_STATUS_R6,
    FB_PIN_STATUS_R7
};

static SemaphoreHandle_t byte_sem;

static uint8_t read_latched_byte(void)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (uint8_t)(gpio_get_level(byte_pins[i]) ? (1u << i) : 0u);
    }
    return v;
}

static inline void ack_byte(void)
{
    /*
     * 74HCT74 /PRE is asynchronous and active low. GPIO39 is open-drain and
     * the line is pulled to +3.3 V externally; HIGH therefore means RELEASE.
     */
    gpio_set_level(FB_PIN_READY_PRE_N, 0);
    __asm__ __volatile__("nop; nop; nop; nop; nop; nop; nop; nop;");
    gpio_set_level(FB_PIN_READY_PRE_N, 1);
}

static void IRAM_ATTR strobe_isr(void *arg)
{
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(byte_sem, &wake);
    if (wake) portYIELD_FROM_ISR();
}

void fb_bus_set_status(uint8_t status)
{
    /* Bit 0 READY belongs to U2, not to ESP32. */
    for (int bit = 1; bit < 8; ++bit) {
        gpio_set_level(status_pins[bit - 1], (status >> bit) & 1u);
    }
}

static bool receive_byte(uint8_t *out, TickType_t timeout)
{
    if (xSemaphoreTake(byte_sem, timeout) != pdTRUE) return false;

    /* The external 574 has held this byte stable since WRITE_N rose. */
    *out = read_latched_byte();

    /* Only after copying it may the C64 send the next byte. */
    ack_byte();
    return true;
}

void fb_bus_init(void)
{
    uint64_t mask = 0;
    for (int i = 0; i < 8; ++i) mask |= 1ULL << byte_pins[i];

    gpio_config_t c = {0};
    c.mode = GPIO_MODE_INPUT;
    c.pin_bit_mask = mask;
    gpio_config(&c);

    mask = 0;
    for (int i = 0; i < 7; ++i) mask |= 1ULL << status_pins[i];
    c = (gpio_config_t){0};
    c.mode = GPIO_MODE_OUTPUT;
    c.pin_bit_mask = mask;
    gpio_config(&c);
    fb_bus_set_status(0);

    c = (gpio_config_t){0};
    c.mode = GPIO_MODE_OUTPUT_OD;
    c.pin_bit_mask = 1ULL << FB_PIN_READY_PRE_N;
    c.pull_up_en = 0; /* external 10 kOhm to +3.3 V */
    gpio_config(&c);
    gpio_set_level(FB_PIN_READY_PRE_N, 1); /* release */

    c = (gpio_config_t){0};
    c.mode = GPIO_MODE_INPUT;
    c.intr_type = GPIO_INTR_POSEDGE;
    c.pin_bit_mask = 1ULL << FB_PIN_WRITE_STROBE;
    c.pull_up_en = 0;
    c.pull_down_en = 0; /* external divider defines the node */
    gpio_config(&c);

    byte_sem = xSemaphoreCreateBinary();

    /* host_bus_init() owns ISR service installation; video adds handlers later. */
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(FB_PIN_WRITE_STROBE, strobe_isr, NULL);

    /* Start with hardware READY=1 after all relevant GPIOs are configured. */
    ack_byte();
}

bool fb_bus_wait_mailbox(fb_mailbox_t *out, uint32_t timeout_ms)
{
    TickType_t first_timeout = timeout_ms == UINT32_MAX
        ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    uint8_t b;

    /* Find packet marker. Each byte is independently flow-controlled. */
    do {
        if (!receive_byte(&b, first_timeout)) return false;
        first_timeout = portMAX_DELAY;
    } while (b != 0xFB);

    /* Six payload bytes follow: command + arg0..arg3 + data. */
    uint8_t *dst = (uint8_t *)out;
    for (unsigned i = 0; i < sizeof(*out); ++i) {
        if (!receive_byte(&dst[i], portMAX_DELAY)) return false;
    }
    return true;
}
