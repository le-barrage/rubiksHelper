#ifndef CAMERA_H
#define CAMERA_H

#include "deps/raylib/src/raylib.h"

#include <stdbool.h>

void cameraOpen (void);

void cameraClose (void);

bool cameraIsActive (void);

bool cameraUpdate (void);

Texture2D cameraGetTexture (void);

Color cameraSampleColor (int cx, int cy, int radius);

#endif  // !CAMERA_H
