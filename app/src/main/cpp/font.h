#ifndef UI_FONT_H
#define UI_FONT_H

#include "raylib.h"

typedef enum {
    FONT_LIGHT,
    FONT_REGULAR,
    FONT_MEDIUM,
    FONT_SEMIBOLD,
    FONT_BOLD,
    FONT_STYLE_COUNT
} FontStyle;

void fontDraw (const char *text, int x, int y, FontStyle style, int size, Color color);

int fontMeasure (const char *text, FontStyle style, int size);

int fontMeasureY (FontStyle style, int size);

Font fontGet (FontStyle style, int size);

void fontShutdown (void);

#endif  // UI_FONT_H
