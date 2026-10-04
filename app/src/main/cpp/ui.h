#ifndef UI_H
#define UI_H

#include "font.h"
#include "raylib.h"

#include <stdbool.h>

bool drawButton (int x, int y, int width, int height, Color color, char *text, Font font);

bool drawButtonPro (int x, int y, int width, int height, Color color, char *text, Font font, int fontSize,
                    Color textColor, float roundness, float segments);

void drawTextBoxed (const char *text, FontStyle style, float font_size, int y);

#endif  // !UI_H
