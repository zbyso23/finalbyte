#ifndef FINALBYTE_H
#define FINALBYTE_H

#include <stdint.h>

/* Weekend Alpha transport: $DF00 is both write-byte and read-status. */
#define FB_PORT   (*(volatile uint8_t*)0xDF00)
#define FB_STATUS (*(volatile uint8_t*)0xDF00)

enum {
    FB_STATUS_READY      = 1u << 0,
    FB_STATUS_BUSY       = 1u << 1,
    FB_STATUS_ERROR      = 1u << 2,
    FB_STATUS_VIDEO_LOCK = 1u << 3,
    FB_STATUS_AUDIO      = 1u << 4,
};

enum {
    FB_CMD_NOP            = 0x00,
    FB_CMD_RESET_FB       = 0x01,
    FB_CMD_SET_VIDEO_MODE = 0x02,
    FB_CMD_CLEAR_FB       = 0x03,
    FB_CMD_DRAW_RECT      = 0x10,
    FB_CMD_SHOW_SPRITE    = 0x11,
    FB_CMD_MOVE_SPRITE    = 0x12,
    FB_CMD_HIDE_SPRITE    = 0x13,
    FB_CMD_PLAY_SAMPLE    = 0x20,
    FB_CMD_STOP_SAMPLE    = 0x21,
    FB_CMD_SET_VOLUME     = 0x22,
};

void fb_wait_ready(void);
void fb_send(uint8_t cmd,
             uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3,
             uint8_t data);

#endif
