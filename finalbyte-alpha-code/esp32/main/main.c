#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fb_protocol.h"
#include "host_bus.h"
#include "video_alpha.h"
#include "audio_alpha.h"

static uint8_t last_error;

static uint8_t current_status(bool busy)
{
    uint8_t s = busy ? FB_STATUS_BUSY : 0;
    if (last_error)        s |= FB_STATUS_ERROR;
    if (fb_video_locked()) s |= FB_STATUS_VIDEO_LOCK;
    if (fb_audio_busy())   s |= FB_STATUS_AUDIO;
    return s;
}

static void execute(const fb_mailbox_t *m)
{
    last_error = 0;

    switch (m->command) {
    case FB_CMD_NOP:
        break;

    case FB_CMD_RESET_FB:
        fb_video_enable(true);
        fb_video_clear(false);
        fb_audio_stop();
        break;

    case FB_CMD_SET_VIDEO_MODE:
        fb_video_enable(m->arg0 != 0);
        break;

    case FB_CMD_CLEAR_FB:
        fb_video_clear(m->arg0 != 0);
        break;

    case FB_CMD_DRAW_RECT:
        fb_video_rect(m->arg0, m->arg1, m->arg2, m->arg3, m->data != 0);
        break;

    case FB_CMD_SHOW_SPRITE:
        fb_video_show_sprite(m->arg0, m->arg1, m->arg2, m->arg3);
        break;

    case FB_CMD_MOVE_SPRITE:
        fb_video_move_sprite(m->arg0, m->arg1, m->arg2);
        break;

    case FB_CMD_HIDE_SPRITE:
        fb_video_hide_sprite(m->arg0);
        break;

    case FB_CMD_PLAY_SAMPLE:
        fb_audio_play(m->arg0, m->arg1);
        break;

    case FB_CMD_STOP_SAMPLE:
        fb_audio_stop();
        break;

    case FB_CMD_SET_VOLUME:
        fb_audio_set_volume(m->arg0);
        break;

    default:
        last_error = 1;
        break;
    }
}

void app_main(void)
{
    fb_bus_init();
    fb_audio_init();
    fb_video_init();

    fb_video_clear(false);
    fb_bus_set_status(current_status(false));

    for (;;) {
        fb_mailbox_t m;
        if (!fb_bus_wait_mailbox(&m, 20)) {
            fb_bus_set_status(current_status(false));
            continue;
        }

        fb_bus_set_status(current_status(true));
        execute(&m);
        fb_bus_set_status(current_status(false));
    }
}
