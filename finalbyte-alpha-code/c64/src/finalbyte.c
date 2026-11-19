#include "finalbyte.h"

void fb_wait_ready(void)
{
    while ((FB_STATUS & FB_STATUS_READY) == 0) {
        /* Hardware READY flip-flop prevents overwrite of the one-byte latch. */
    }
}

static void fb_put(uint8_t value)
{
    fb_wait_ready();
    FB_PORT = value;
}

void fb_send(uint8_t cmd,
             uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3,
             uint8_t data)
{
    fb_put(0xFB);       /* packet marker */
    fb_put(cmd);
    fb_put(a0);
    fb_put(a1);
    fb_put(a2);
    fb_put(a3);
    fb_put(data);
}
