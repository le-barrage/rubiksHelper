#include "ui.h"

#include "font.h"
#include "raylib.h"

#include <string.h>

#define BUF_LEN    256
#define MAX_TOKENS 64
#define MAX_LINES  16

static inline int min (int a, int b) { return a < b ? a : b; }

bool drawButtonPro (int x, int y, int width, int height, Color color, char *text, Font font, int fontSize,
                    Color textColor, float roundness, float segments)
{
    DrawRectangleRounded((Rectangle){ x, y, width, height }, roundness, segments, color);

    int spacing       = min(fontSize / 10, 4);
    Vector2 textWidth = MeasureTextEx(font, text, fontSize, spacing);
    int textX         = x + (width - textWidth.x) / 2.f;
    int textY         = y + (height - textWidth.y) / 2.f;
    DrawTextEx(font, text, (Vector2){ textX, textY }, fontSize, spacing, textColor);

    int mouse_x = GetMouseX(), mouse_y = GetMouseY();
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && x <= mouse_x && mouse_x <= x + width && y <= mouse_y
           && mouse_y <= y + height;
}

bool drawButton (int x, int y, int width, int height, Color color, char *text, Font font)
{
    return drawButtonPro(x, y, width, height, color, text, font, 50, BLACK, 0.2, 1.f);
}

void drawTextBoxed (const char *text, FontStyle style, float font_size, int y)
{
    if (strlen(text) == 0) return;

    char work_copy[BUF_LEN];
    strncpy(work_copy, text, sizeof(work_copy) - 1);
    work_copy[sizeof(work_copy) - 1] = '\0';

    char *tokens[MAX_TOKENS];
    int token_count = 0;
    for (char *t = strtok(work_copy, " "); t && token_count < MAX_TOKENS; t = strtok(NULL, " "))
        tokens[token_count++] = t;
    if (token_count == 0) return;

    int max_width = GetScreenWidth() - 100;

    int line_start[MAX_LINES] = { 0 };
    int line_count            = 1;

    char line_buf[BUF_LEN];
    int len0 = strlen(tokens[0]);
    memcpy(line_buf, tokens[0], len0);
    int line_len       = len0;
    line_buf[line_len] = '\0';

    for (int i = 1; i < token_count; i++) {
        int tlen = strlen(tokens[i]);
        if (line_len + 1 + tlen + 1 > (int)sizeof(line_buf)) break;
        line_buf[line_len] = ' ';
        memcpy(line_buf + line_len + 1, tokens[i], tlen);
        line_buf[line_len + 1 + tlen] = '\0';

        if (fontMeasure(line_buf, style, font_size) > max_width && line_count < MAX_LINES) {
            line_buf[line_len]       = '\0';
            line_start[line_count++] = i;
            memcpy(line_buf, tokens[i], tlen);
            line_len           = tlen;
            line_buf[line_len] = '\0';
        } else
            line_len += 1 + tlen;
    }
    line_start[line_count] = token_count;

    for (int line = 0; line < line_count; line++) {
        int start_idx = line_start[line];
        int end_idx   = line_start[line + 1];

        char buf[BUF_LEN];
        int len = 0;
        for (int i = start_idx; i < end_idx; i++) {
            if (i > start_idx && len + 1 < (int)sizeof(buf)) buf[len++] = ' ';
            int tlen = strlen(tokens[i]);
            if (len + tlen >= (int)sizeof(buf)) break;
            memcpy(buf + len, tokens[i], tlen);
            len += tlen;
        }
        buf[len] = '\0';

        int line_width = fontMeasure(buf, style, font_size);
        int line_x     = (GetScreenWidth() - line_width) / 2;
        int line_y     = y + line * fontMeasureY(style, font_size);

        fontDraw(buf, line_x, line_y, style, font_size, RAYWHITE);
    }
}
