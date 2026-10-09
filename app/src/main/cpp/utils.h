#ifndef UTILS_H
#define UTILS_H

#include "deps/raylib/src/raylib.h"
#include "kociemba/enums.h"

bool colorEquals (Color c1, Color c2);

char getNotationFromColor (Color c);

char getNotationFromIndex (int index);

char getOrientationChar (face_t orientation);

char getDirectionChar (Direction direction);

Color classifyColor (Color c);

#endif  // !UTILS_H
