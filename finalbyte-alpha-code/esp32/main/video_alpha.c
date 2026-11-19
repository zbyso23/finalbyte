#include "video_alpha.h"
#include "board_pins.h"
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

// Alpha deliberately uses a 256x200 1-bit mask. 256 pixels keeps each row at
// exactly 32 bytes and is enough to prove synchronized binary overlay.
#define ROW_BYTES (FB_WIDTH / 8)

// Clean 3.3 V pulses from the external sync-recovery circuit.

// SPI MOSI is the binary host/FINALBYTE select signal. External analog logic
// interprets 0=host video, 1=FINALBYTE fixed-luma pixel.

// Bring-up calibration values. These are intentionally compile-time constants
// until the real board is measured with an oscilloscope.
#define VISIBLE_FIRST_PAL_LINE 50
#define ACTIVE_DELAY_US        10
#define PIXEL_CLOCK_HZ         6000000

static uint8_t fb[FB_HEIGHT][ROW_BYTES];
static volatile uint16_t pal_line;
static volatile bool video_enabled;
static volatile int64_t last_vsync_us;
static volatile int pending_row = -1;
static TaskHandle_t video_task_handle;
static spi_device_handle_t spi_dev;

typedef struct {
    bool visible;
    uint8_t x, y;
    uint8_t frame;
} sprite_t;

static sprite_t sprite0;

static const uint16_t sprite_bits[16] = {
    0x0180,0x03C0,0x07E0,0x0FF0,
    0x1DB8,0x399C,0x718E,0xE187,
    0xE187,0x718E,0x399C,0x1DB8,
    0x0FF0,0x07E0,0x03C0,0x0180
};

static inline void set_pixel(int x, int y, bool on)
{
    if ((unsigned)x >= FB_WIDTH || (unsigned)y >= FB_HEIGHT) return;
    uint8_t mask = (uint8_t)(0x80u >> (x & 7));
    if (on) fb[y][x >> 3] |= mask;
    else    fb[y][x >> 3] &= (uint8_t)~mask;
}

static void draw_sprite0(bool on)
{
    if (!sprite0.visible) return;
    for (int yy = 0; yy < 16; ++yy) {
        uint16_t row = sprite_bits[yy];
        for (int xx = 0; xx < 16; ++xx) {
            if (row & (0x8000u >> xx)) set_pixel(sprite0.x + xx, sprite0.y + yy, on);
        }
    }
}

static void IRAM_ATTR vsync_isr(void *arg)
{
    pal_line = 0;
    last_vsync_us = esp_timer_get_time();
}

static void IRAM_ATTR hsync_isr(void *arg)
{
    uint16_t line = pal_line++;
    if (!video_enabled) return;

    if (line >= VISIBLE_FIRST_PAL_LINE &&
        line < VISIBLE_FIRST_PAL_LINE + FB_HEIGHT) {
        pending_row = (int)(line - VISIBLE_FIRST_PAL_LINE);
        BaseType_t wake = pdFALSE;
        vTaskNotifyGiveFromISR(video_task_handle, &wake);
        if (wake) portYIELD_FROM_ISR();
    }
}

static void video_task(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        int row = pending_row;
        if ((unsigned)row >= FB_HEIGHT || !video_enabled) continue;

        // Horizontal phase is calibrated against the actual C64/sync separator.
        // This task is pinned at high priority and performs no allocation.
        esp_rom_delay_us(ACTIVE_DELAY_US);

        spi_transaction_t t = {0};
        t.length = FB_WIDTH;
        t.tx_buffer = fb[row];
        spi_device_polling_transmit(spi_dev, &t);
    }
}

void fb_video_init(void)
{
    memset(fb, 0, sizeof(fb));

    spi_bus_config_t bus = {
        .mosi_io_num = FB_PIN_MASK_OUT,
        .miso_io_num = -1,
        .sclk_io_num = FB_PIN_MASK_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ROW_BYTES,
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = PIXEL_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = FB_PIN_MASK_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &dev, &spi_dev);

    xTaskCreatePinnedToCore(video_task, "fb_video", 3072, NULL,
                            configMAX_PRIORITIES - 2, &video_task_handle, 0);

    gpio_config_t c = {0};
    c.mode = GPIO_MODE_INPUT;
    c.intr_type = GPIO_INTR_POSEDGE;
    c.pin_bit_mask = (1ULL << FB_PIN_HSYNC) | (1ULL << FB_PIN_VSYNC);
    gpio_config(&c);

    gpio_isr_handler_add(FB_PIN_HSYNC, hsync_isr, NULL);
    gpio_isr_handler_add(FB_PIN_VSYNC, vsync_isr, NULL);

    last_vsync_us = 0;
    video_enabled = true;
}

void fb_video_enable(bool enable)
{
    video_enabled = enable;
}

void fb_video_clear(bool finalbyte_pixel)
{
    memset(fb, finalbyte_pixel ? 0xFF : 0x00, sizeof(fb));
    sprite0.visible = false;
}

void fb_video_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool finalbyte_pixel)
{
    for (int yy = y; yy < (int)y + h && yy < FB_HEIGHT; ++yy)
        for (int xx = x; xx < (int)x + w && xx < FB_WIDTH; ++xx)
            set_pixel(xx, yy, finalbyte_pixel);
}

void fb_video_show_sprite(uint8_t id, uint8_t x, uint8_t y, uint8_t frame)
{
    if (id != 0) return;
    if (sprite0.visible) draw_sprite0(false);
    sprite0 = (sprite_t){ .visible=true, .x=x, .y=y, .frame=frame };
    draw_sprite0(true);
}

void fb_video_move_sprite(uint8_t id, uint8_t x, uint8_t y)
{
    if (id != 0) return;
    if (sprite0.visible) draw_sprite0(false);
    sprite0.x = x;
    sprite0.y = y;
    sprite0.visible = true;
    draw_sprite0(true);
}

void fb_video_hide_sprite(uint8_t id)
{
    if (id != 0 || !sprite0.visible) return;
    draw_sprite0(false);
    sprite0.visible = false;
}

bool fb_video_locked(void)
{
    return last_vsync_us != 0 && (esp_timer_get_time() - last_vsync_us) < 50000;
}
