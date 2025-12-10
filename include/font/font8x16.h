#pragma once

#include <stdint.h>

#define FONT8X16_WIDTH   8
#define FONT8X16_HEIGHT  16
#define FONT8X16_FIRST   0x00
#define FONT8X16_LAST    0x7F
#define FONT8X16_COUNT   (FONT8X16_LAST - FONT8X16_FIRST + 1)

extern const uint8_t font8x16[FONT8X16_COUNT][FONT8X16_HEIGHT];