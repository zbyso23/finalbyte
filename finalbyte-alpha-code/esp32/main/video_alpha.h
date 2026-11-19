#pragma once
#include <stdbool.h>
#include <stdint.h>

#define FB_WIDTH  256
#define FB_HEIGHT 200

void fb_video_init(void);
void fb_video_enable(bool enable);
void fb_video_clear(bool finalbyte_pixel);
void fb_video_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool finalbyte_pixel);
void fb_video_show_sprite(uint8_t id, uint8_t x, uint8_t y, uint8_t frame);
void fb_video_move_sprite(uint8_t id, uint8_t x, uint8_t y);
void fb_video_hide_sprite(uint8_t id);
bool fb_video_locked(void);
