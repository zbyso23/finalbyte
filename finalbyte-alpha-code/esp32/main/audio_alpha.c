#include "audio_alpha.h"
#include "board_pins.h"
#include <math.h>
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLE_RATE 16000
static i2s_chan_handle_t tx_chan;
static volatile bool play_request;
static volatile bool stop_request;
static volatile bool busy;
static volatile uint8_t requested_sample;
static volatile uint8_t global_volume = 192;

static int16_t scale_sample(int16_t s, uint8_t v)
{
    return (int16_t)(((int32_t)s * v) / 255);
}

static void audio_task(void *arg)
{
    int16_t block[128 * 2]; // stereo interleaved

    for (;;) {
        if (!play_request) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        play_request = false;
        stop_request = false;
        busy = true;

        uint8_t id = requested_sample;
        const float f1 = (id == 0) ? 660.0f : 440.0f;
        const float f2 = (id == 0) ? 990.0f : 880.0f;
        const int total = SAMPLE_RATE / 3;

        for (int base = 0; base < total && !stop_request; base += 128) {
            int n = total - base;
            if (n > 128) n = 128;

            for (int i = 0; i < n; ++i) {
                int p = base + i;
                float t = (float)p / SAMPLE_RATE;
                float env = 1.0f - (float)p / total;
                float s = 0.55f * sinf(2.0f * 3.14159265358979323846f * f1 * t)
                        + 0.25f * sinf(2.0f * 3.14159265358979323846f * f2 * t);
                int16_t v = scale_sample((int16_t)(s * env * 28000.0f), global_volume);
                block[i*2+0] = v;
                block[i*2+1] = v;
            }

            size_t bytes = 0;
            i2s_channel_write(tx_chan, block, (size_t)n * 2 * sizeof(int16_t), &bytes, portMAX_DELAY);
        }

        busy = false;
    }
}

void fb_audio_init(void)
{
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&cc, &tx_chan, NULL);

    i2s_std_config_t sc = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = FB_PIN_I2S_BCLK,
            .ws = FB_PIN_I2S_WS,
            .dout = FB_PIN_I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {0},
        },
    };
    i2s_channel_init_std_mode(tx_chan, &sc);
    i2s_channel_enable(tx_chan);

    xTaskCreatePinnedToCore(audio_task, "fb_audio", 4096, NULL, 5, NULL, 1);
}

void fb_audio_set_volume(uint8_t volume) { global_volume = volume; }
void fb_audio_play(uint8_t sample_id, uint8_t volume)
{
    requested_sample = sample_id;
    global_volume = volume;
    play_request = true;
}
void fb_audio_stop(void) { stop_request = true; }
bool fb_audio_busy(void) { return busy; }
