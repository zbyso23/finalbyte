#pragma once

#include "driver/gpio.h"

/*
 * FINALBYTE Weekend Alpha Rev A.2 — single source of truth for ESP32-S3 pins.
 * Must match FINALBYTE-Alpha-Weekend-HW-RevA1.md / NETLIST.csv.
 *
 * GPIO1..2   sync inputs after 5V->3.3V dividers
 * GPIO3      1-bit video mask (SPI MOSI)
 * GPIO4..11  latched C64 byte from SN74LVC574A Q0..Q7
 * GPIO12     WRITE_N positive-edge strobe after 10k/20k divider
 * GPIO13..18 status bits 1..6 toward 74AHCT245
 * GPIO21     status bit 7 toward 74AHCT245
 * GPIO39     READY_PRE_N open-drain, external 10k pull-up to +3.3V
 * GPIO42     mask SPI clock
 * GPIO43     mask SPI CS, active low; hardware inverts to MASK_ACTIVE
 * GPIO44     I2S BCLK
 * GPIO47     I2S WS/LRCK
 * GPIO48     I2S DATA
 */

#define FB_PIN_HSYNC       GPIO_NUM_1
#define FB_PIN_VSYNC       GPIO_NUM_2
#define FB_PIN_MASK_OUT    GPIO_NUM_3

#define FB_PIN_BUS_D0      GPIO_NUM_4
#define FB_PIN_BUS_D1      GPIO_NUM_5
#define FB_PIN_BUS_D2      GPIO_NUM_6
#define FB_PIN_BUS_D3      GPIO_NUM_7
#define FB_PIN_BUS_D4      GPIO_NUM_8
#define FB_PIN_BUS_D5      GPIO_NUM_9
#define FB_PIN_BUS_D6      GPIO_NUM_10
#define FB_PIN_BUS_D7      GPIO_NUM_11
#define FB_PIN_WRITE_STROBE GPIO_NUM_12

#define FB_PIN_STATUS_BUSY       GPIO_NUM_13
#define FB_PIN_STATUS_ERROR      GPIO_NUM_14
#define FB_PIN_STATUS_VIDEO_LOCK GPIO_NUM_15
#define FB_PIN_STATUS_AUDIO      GPIO_NUM_16
#define FB_PIN_STATUS_R5         GPIO_NUM_17
#define FB_PIN_STATUS_R6         GPIO_NUM_18
#define FB_PIN_STATUS_R7         GPIO_NUM_21

#define FB_PIN_READY_PRE_N  GPIO_NUM_39
#define FB_PIN_MASK_CLK     GPIO_NUM_42
#define FB_PIN_MASK_CS      GPIO_NUM_43
#define FB_PIN_I2S_BCLK     GPIO_NUM_44
#define FB_PIN_I2S_WS       GPIO_NUM_47
#define FB_PIN_I2S_DOUT     GPIO_NUM_48
