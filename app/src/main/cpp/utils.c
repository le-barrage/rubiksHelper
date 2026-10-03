#include "utils.h"

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
    return '?';
}
