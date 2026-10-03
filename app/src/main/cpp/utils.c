#include "utils.h"

#include "raylib.h"

#include <stdbool.h>

bool colorEquals (Color c1, Color c2) { return c1.a == c2.a && c1.r == c2.r && c1.g == c2.g && c1.b == c2.b; }

char getNotationFromColor (Color c)
{
    if (colorEquals(c, WHITE))
        return 'U';
    else if (colorEquals(c, ORANGE))
        return 'L';
    else if (colorEquals(c, GREEN))
        return 'F';
    else if (colorEquals(c, RED))
        return 'R';
    else if (colorEquals(c, YELLOW))
        return 'D';
    else if (colorEquals(c, BLUE))
        return 'B';
    TraceLog(LOG_ERROR, "UTILS: Unknown RGB %d %d %d", c.r, c.g, c.b);
    return '?';
}

char getOrientationChar (face_t orientation)
{
    switch (orientation) {
        case FACE_UP:
            return 'U';
        case FACE_LEFT:
            return 'L';
        case FACE_FRONT:
            return 'F';
        case FACE_RIGHT:
            return 'R';
        case FACE_DOWN:
            return 'D';
        case FACE_BACK:
            return 'B';
        default:
            TraceLog(LOG_ERROR, "UTILS: Unknown orientation %d", orientation);
            return '?';
    }
}

char getDirectionChar (Direction direction)
{
    switch (direction) {
        case ANTICW:
            return '\'';
        case HALF:
            return '2';
        case CW:
            return '.';
        case NONE:
        default:
            TraceLog(LOG_ERROR, "UTILS: Unknown direction %d", direction);
            return '?';
    }
}
