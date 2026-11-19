#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "fb_protocol.h"

void fb_bus_init(void);
bool fb_bus_wait_mailbox(fb_mailbox_t *out, uint32_t timeout_ms);
void fb_bus_set_status(uint8_t status);
