#include "utils.h"

#include "raylib.h"
#include "raymob.h"

#include <stdbool.h>

bool backPressed (void)
{
    bool pressed = false;
    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed())
        if (key == KEY_BACK) pressed = true;
    return pressed;
}

void moveAppToBackground (void)
{
    JNIEnv *env        = AttachCurrentThread();
    jobject activity   = GetNativeLoaderInstance();
    jclass cls         = (*env)->GetObjectClass(env, activity);
    jmethodID moveTask = (*env)->GetMethodID(env, cls, "moveTaskToBack", "(Z)Z");
    (*env)->CallBooleanMethod(env, activity, moveTask, JNI_TRUE);
    (*env)->DeleteLocalRef(env, cls);
    DetachCurrentThread();
}

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

char getNotationFromIndex (int index)
{
    switch (index) {
        case 0:
            return 'U';
        case 1:
            return 'L';
        case 2:
            return 'F';
        case 3:
            return 'R';
        case 4:
            return 'D';
        case 5:
            return 'B';
        default:
            return '?';
    }
}

char *getColorNameFromNotation (char face)
{
    switch (face) {
        case 'U':
            return "white";
        case 'L':
            return "orange";
        case 'F':
            return "green";
        case 'R':
            return "red";
        case 'D':
            return "yellow";
        case 'B':
            return "blue";
        default:
            TraceLog(LOG_ERROR, "UTILS: Unknown face %c", face);
            return "?";
    }
}

char getTopNotationFromNotation (char face)
{
    switch (face) {
        case 'U':
            return 'B';
        case 'L':
        case 'F':
        case 'R':
            return 'U';
        case 'D':
            return 'F';
        case 'B':
            return 'D';
        default:
            TraceLog(LOG_ERROR, "UTILS: Unknown face %c", face);
            return '?';
    }
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

Color classifyColor (Color c)
{
    Vector3 hsv = ColorToHSV(c);

    if (hsv.y < 0.25f && hsv.z > 0.5f) return WHITE;

    float h = hsv.x;
    if (h < 8 || h >= 330) return RED;

    if (h < 40) return ORANGE;
    if (h < 75) return YELLOW;
    if (h < 170) return GREEN;
    if (h < 260) return BLUE;
    return RED;
}
