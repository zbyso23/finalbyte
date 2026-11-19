#include <stdint.h>
#include <conio.h>
#include <peekpoke.h>
#include "finalbyte.h"

#define VIC_BORDER 0xD020
#define VIC_BG     0xD021

#define SID_FREQ_LO 0xD400
#define SID_FREQ_HI 0xD401
#define SID_CTRL    0xD404
#define SID_AD      0xD405
#define SID_SR      0xD406
#define SID_VOL     0xD418

static void delay(unsigned int n)
{
    volatile unsigned int i;
    while (n--) {
        for (i = 0; i < 350; ++i) { }
    }
}

static void sid_beep(uint16_t freq, uint8_t frames)
{
    POKE(SID_FREQ_LO, (uint8_t)(freq & 0xff));
    POKE(SID_FREQ_HI, (uint8_t)(freq >> 8));
    POKE(SID_AD, 0x11);
    POKE(SID_SR, 0xF1);
    POKE(SID_VOL, 0x0F);
    POKE(SID_CTRL, 0x21); /* saw + gate */
    delay(frames);
    POKE(SID_CTRL, 0x20);
}

static void host_draw_marker(uint8_t x)
{
    uint8_t i;
    gotoxy(0, 18);
    for (i = 0; i < 40; ++i) cputc(' ');
    gotoxy((uint8_t)(x >> 2), 18);
    cputc('*');
}

static void setup_finalbyte(void)
{
    fb_send(FB_CMD_RESET_FB, 0, 0, 0, 0, 0);
    fb_send(FB_CMD_SET_VIDEO_MODE, 1, 0, 0, 0, 0);
    fb_send(FB_CMD_CLEAR_FB, 0, 0, 0, 0, 0);

    /* FINALBYTE frame around the center of the C64 screen. */
    fb_send(FB_CMD_DRAW_RECT, 24, 40, 208, 2, 0, 1);
    fb_send(FB_CMD_DRAW_RECT, 24, 156, 208, 2, 0, 1);
    fb_send(FB_CMD_DRAW_RECT, 24, 40, 2, 118, 0, 1);
    fb_send(FB_CMD_DRAW_RECT, 230, 40, 2, 118, 0, 1);

    /* Built-in 16x16 FINALBYTE test sprite. */
    fb_send(FB_CMD_SHOW_SPRITE, 0, 32, 92, 0, 0);
}

int main(void)
{
    uint8_t x = 32;
    int8_t dx = 2;
    uint8_t tick = 0;
    uint8_t key;

    POKE(VIC_BORDER, 6);
    POKE(VIC_BG, 0);
    clrscr();
    textcolor(1);

    cputsxy(4, 2, "FINALBYTE ALPHA / C64");
    cputsxy(2, 5, "C64: TEXT + SID + GAME LOGIC");
    cputsxy(2, 7, "FINALBYTE: FRAME + MOVING SPRITE");
    cputsxy(2, 10, "SPACE = C64 BEEP + FB SAMPLE");
    cputsxy(2, 12, "Q = QUIT / FB PASSTHROUGH");

    setup_finalbyte();
    sid_beep(0x2200, 2);

    for (;;) {
        /* The C64 owns the motion/game state. FINALBYTE only renders it. */
        x = (uint8_t)(x + dx);
        if (x >= 210) dx = -2;
        if (x <= 32)  dx = 2;

        host_draw_marker(x);
        fb_send(FB_CMD_MOVE_SPRITE, 0, x, 92, 0, 0);

        ++tick;
        if ((tick & 31) == 0) {
            /* C64 proves it is alive independently of FINALBYTE. */
            POKE(VIC_BORDER, (PEEK(VIC_BORDER) + 1) & 0x0F);
        }

        key = 0;
        if (kbhit()) key = cgetc();
        if (key == ' ') {
            sid_beep(0x3300, 1);
            fb_send(FB_CMD_PLAY_SAMPLE, 0, 220, 0, 0, 0);
        } else if (key == 'q' || key == 'Q') {
            break;
        }

        delay(1);
    }

    fb_send(FB_CMD_HIDE_SPRITE, 0, 0, 0, 0, 0);
    fb_send(FB_CMD_CLEAR_FB, 0, 0, 0, 0, 0);
    fb_send(FB_CMD_SET_VIDEO_MODE, 0, 0, 0, 0, 0);
    POKE(SID_CTRL, 0);
    POKE(SID_VOL, 0);
    clrscr();
    cputs("FINALBYTE DEMO ENDED.\r\n");
    return 0;
}
