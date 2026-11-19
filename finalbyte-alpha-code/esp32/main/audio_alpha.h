#pragma once
#include <stdint.h>
#include <stdbool.h>

void fb_audio_init(void);
void fb_audio_set_volume(uint8_t volume);
void fb_audio_play(uint8_t sample_id, uint8_t volume);
void fb_audio_stop(void);
bool fb_audio_busy(void);
