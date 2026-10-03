#include "ui.h"

bool drawButton (int x, int y, int width, int height, Color color, char *text, Font font)
{
    DrawRectangleRounded((Rectangle){ x, y, width, height }, 0.2, 1, color);

    Vector2 textWidth = MeasureTextEx(font, text, 50, 3);
    int textX         = x + (width - textWidth.x) / 2.f;
    int textY         = y + (height - textWidth.y) / 2.f;
    DrawTextEx(font, text, (Vector2){ textX, textY }, 50, 3, BLACK);

    int mouse_x = GetMouseX(), mouse_y = GetMouseY();
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && x <= mouse_x && mouse_x <= x + width && y <= mouse_y
           && mouse_y <= y + height;
}
