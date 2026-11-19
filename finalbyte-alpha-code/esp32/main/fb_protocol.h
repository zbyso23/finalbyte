#pragma once
#include <stdint.h>

#define FB_VERSION_MAJOR 1
#define FB_VERSION_MINOR 3
#define FB_DEVICE_ID     0xA1

// Weekend Rev A.2 physical transport:
//   write $DF00 = one byte to FINALBYTE
//   read  $DF00 = status byte
// STATUS bits (read at $DF00 on C64)
enum {
    FB_STATUS_READY      = 1u << 0,
    FB_STATUS_BUSY       = 1u << 1,
    FB_STATUS_ERROR      = 1u << 2,
    FB_STATUS_VIDEO_LOCK = 1u << 3,
    FB_STATUS_AUDIO      = 1u << 4,
};

// Minimal Alpha command set.
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

typedef struct {
    uint8_t command;
    uint8_t arg0;
    uint8_t arg1;
    uint8_t arg2;
    uint8_t arg3;
    uint8_t data;
} fb_mailbox_t;

// Command semantics used by the Alpha demo:
//
// RESET_FB
//   no args
//
// SET_VIDEO_MODE
//   arg0: 0 = passthrough, 1 = overlay enabled
//
// CLEAR_FB
//   arg0: 0 = all host pixels, 1 = all FINALBYTE pixels
//
// DRAW_RECT
//   arg0=x, arg1=y, arg2=width, arg3=height
//   data: 0 = host pixel, 1 = FINALBYTE pixel
//
// SHOW_SPRITE
//   arg0=id, arg1=x, arg2=y, arg3=frame
//   built-in Alpha sprite only
//
// MOVE_SPRITE
//   arg0=id, arg1=x, arg2=y
//
// HIDE_SPRITE
//   arg0=id
//
// PLAY_SAMPLE
//   arg0=sample id, arg1=volume 0..255
//
// STOP_SAMPLE
//   no args
//
// SET_VOLUME
//   arg0=0..255
