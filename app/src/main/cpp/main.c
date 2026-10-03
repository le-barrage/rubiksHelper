#include "deps/raylib/src/raylib.h"
#include "kociemba/coordCube.h"
#include "kociemba/enums.h"
#include "kociemba/twoPhase.h"
#include "ui.h"
#include "utils.h"

#include <stdbool.h>
#include <stdio.h>

#define START_X GetScreenWidth() / 2. - squareSize * 1.5
#define START_Y 250

typedef struct {
    int x;
    int y;
} GridCoord;

Font font;

Color faces[6][9]     = { 0 };
GridCoord facesPos[6] = { 0 };
Color colors[6]       = { WHITE, ORANGE, GREEN, RED, YELLOW, BLUE };

int selectedFace          = -1;
GridCoord selectedFacelet = { -1, -1 };

int squareSize = 135;
int faceMargin = 20;

char str[54]   = { 0 };
Move moves[30] = { 0 };
int depth      = -1;
int err        = 0;
bool good      = false;

bool debug = false;

/*             |************|
 *             |*U1**U2**U3*|
 *             |************|
 *             |*U4**U5**U6*|
 *             |************|
 *             |*U7**U8**U9*|
 *             |************|
 * ************|************|************|
 * *L1**L2**L3*|*F1**F2**F3*|*R1**R2**F3*|
 * ************|************|************|
 * *L4**L5**L6*|*F4**F5**F6*|*R4**R5**R6*|
 * ************|************|************|
 * *L7**L8**L9*|*F7**F8**F9*|*R7**R8**R9*|
 * ************|************|************|
 *             |************|
 *             |*D1**D2**D3*|
 *             |************|
 *             |*D4**D5**D6*|
 *             |************|
 *             |*D7**D8**D9*|
 *             |************|
 *             |************|
 *             |*B9**B8**B7*|
 *             |************|
 *             |*B6**B5**B4*|
 *             |************|
 *             |*B3**B2**B1*|
 *             |************|
 */
static void toStr (char *out)
{
    // URFDLB => 032415
    int URFDLB[] = { 0, 3, 2, 4, 1, 5 };
    int curr     = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                out[curr++] = getNotationFromColor(faces[URFDLB[i]][k * 3 + j]);
            }
        }
    }
    for (int j = 2; j >= 0; j--) {
        for (int k = 2; k >= 0; k--) {
            out[curr++] = getNotationFromColor(faces[URFDLB[5]][k * 3 + j]);
        }
    }
}

static void printMoves ()
{
    char str[depth * 3];
    int curr = 0;
    for (int i = 0; i < depth; i++) {
        str[curr++] = getOrientationChar(moves[i].orientation);
        str[curr++] = getDirectionChar(moves[i].direction);
        str[curr++] = ' ';
    }
    str[curr++] = '\0';

    Vector2 textSize = MeasureTextEx(font, str, 50, 4);
    DrawTextEx(font, str, (Vector2){ (GetScreenWidth() - textSize.x) / 2, 2500 }, 50, 4, RAYWHITE);
}

static void draw3x3 (int x, int y, int index)
{
    DrawRectangle(x - 5, y - 5, squareSize * 3 + 10, squareSize * 3 + 10, GRAY);
    for (int j = 0; j < 3; j++) {
        int squareY = y + j * squareSize;
        for (int i = 0; i < 3; i++) {
            int squareX = x + i * squareSize;

            DrawRectangle(squareX, squareY, squareSize, squareSize, faces[index][i * 3 + j]);
            DrawRectangleLines(squareX, squareY, squareSize, squareSize, BLACK);

            if (debug) {
                char num[2];
                sprintf(num, "%d", j * 3 + i);
                Vector2 textWidth = MeasureTextEx(font, num, 30, 3);
                int textX         = squareX + (squareSize - textWidth.x) / 2;
                int textY         = squareY + (squareSize - 20) / 2;
                DrawTextEx(font, num, (Vector2){ textX, textY }, 30, 3, BLACK);
            }
        }
    }
}

int getFaceTouched (int px, int py)
{
    int gridStartX = GetScreenWidth() / 2. - (squareSize * 4.5 + faceMargin);
    int spacing    = squareSize * 3 + faceMargin;

    int offsetX = px - gridStartX;
    int offsetY = py - START_Y;

    if (offsetX < 0 || offsetY < 0) return -1;

    // Check if click is in a margin between squares
    int marginX = offsetX % spacing;
    int marginY = offsetY % spacing;

    if (marginX >= squareSize * 3 || marginY >= squareSize * 3) return -1;

    GridCoord coord = { .x = offsetX / spacing, .y = offsetY / spacing };

    if (coord.y == 0) return (coord.x == 1) ? 0 : -1;

    if (coord.y == 1 && coord.x < 3) return coord.x + 1;

    if (coord.y == 2) return (coord.x == 1) ? 4 : -1;

    if (coord.y == 3) return (coord.x == 1) ? 5 : -1;

    return -1;
}

static void resetColors ()
{
    for (int i = 0; i < 6; i++) {
        Color currColor = colors[i];
        for (int j = 0; j < 9; j++) faces[i][j] = currColor;
    }
}

static void initFacePos ()
{
    facesPos[0] = (GridCoord){ START_X, START_Y };
    facesPos[1] = (GridCoord){ START_X - 3 * squareSize - faceMargin, START_Y + 3 * squareSize + faceMargin };
    facesPos[2] = (GridCoord){ START_X, START_Y + 3 * squareSize + faceMargin };
    facesPos[3] = (GridCoord){ START_X + 3 * squareSize + faceMargin, START_Y + 3 * squareSize + faceMargin };
    facesPos[4] = (GridCoord){ START_X, START_Y + 2 * (3 * squareSize + faceMargin) };
    facesPos[5] = (GridCoord){ START_X, START_Y + 3 * (3 * squareSize + faceMargin) };
}

int main (void)
{
    init();
    InitWindow(GetScreenWidth(), GetScreenHeight(), "rubiksHelper");
    SetTargetFPS(60);

    font = LoadFont("SpaceGrotesk/SpaceGrotesk-SemiBold.ttf");

    resetColors();
    initFacePos();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));

        if (drawButton(GetScreenWidth() - 200, 50, 150, 100, GetColor(0x666666FF), "DEBUG", font)) debug = !debug;

        if (drawButton(100, 50, 150, 100, GetColor(0x666666FF), "RESET", font)) {
            resetColors();
            depth = 0;
        }

        int x = GetScreenWidth() / 2. - squareSize * 3 - faceMargin * 2.5;
        for (int i = 0; i < 6; i++) {
            if (drawButton(x, 2800, squareSize, squareSize, colors[i], "", font))
                faces[selectedFace][selectedFacelet.x * 3 + selectedFacelet.y] = colors[i];
            x += squareSize + faceMargin;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {}
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            int mouse_x = GetMouseX(), mouse_y = GetMouseY();
            selectedFace = getFaceTouched(mouse_x, mouse_y);
            if (selectedFace != -1) {
                selectedFacelet.x = (mouse_x - facesPos[selectedFace].x) / squareSize;
                selectedFacelet.y = (mouse_y - facesPos[selectedFace].y) / squareSize;
            } else
                selectedFacelet = (GridCoord){ -1, -1 };
            if (debug) {
                DrawTextEx(font, TextFormat("DOWN %d", selectedFace), (Vector2){ 100, 100 }, 50, 5, BLACK);
                DrawText(TextFormat("DOWN %d", selectedFace), 100, 100, 50, BLACK);
                DrawTextEx(font, TextFormat("DOWN %d %d", selectedFacelet.x, selectedFacelet.y), (Vector2){ 100, 150 },
                           50, 5, BLACK);
                DrawTextEx(font, TextFormat("DOWN %d %d", GetMouseX(), GetMouseY()), (Vector2){ 100, 200 }, 50, 5,
                           BLACK);
            }
        }

        for (int i = 0; i < 6; i++) {
            int x = facesPos[i].x, y = facesPos[i].y;
            draw3x3(x, y, i);
            if (selectedFace == i) {
                DrawRectangleLinesEx((Rectangle){ x, y, squareSize * 3, squareSize * 3 }, 5, BLACK);
                DrawRectangleLinesEx((Rectangle){ selectedFacelet.x * squareSize + facesPos[i].x,
                                                  selectedFacelet.y * squareSize + facesPos[i].y, squareSize,
                                                  squareSize },
                                     3, BLACK);
            }
        }

        if (drawButton(GetScreenWidth() / 2 - 150, 2200, 300, 125, GetColor(0x9370DBFF), "SOLVE", font)) {
            toStr(str);
            err  = findSolutionBasic(str, 25, 5000, moves, &depth);
            good = true;
        }
        if (err) {
            char *errMsg      = printErrorMessage(err);
            Vector2 textWidth = MeasureTextEx(font, errMsg, 50, 4);
            DrawTextEx(font, errMsg, (Vector2){ (GetScreenWidth() - textWidth.x) / 2, 2600 }, 50, 4, RED);
        } else if (!err && good)
            printMoves();

        if (debug) DrawLine(GetScreenWidth() / 2, 0, GetScreenWidth() / 2, GetScreenHeight(), BLACK);

        EndDrawing();
    }
    cameraClose();
    UnloadFont(font);

    CloseWindow();

    return 0;
}
