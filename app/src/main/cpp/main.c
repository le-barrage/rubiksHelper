#include "camera.h"
#include "deps/raylib/src/raylib.h"
#include "font.h"
#include "kociemba/coordCube.h"
#include "kociemba/twoPhase.h"
#include "raymob.h"
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

Font fontBold;
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

int scanFace;

#define SCREEN_WHITE_FRAMES 5
int screenWhite = 0;

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

static void printMoves (int y)
{
    if (depth == 0) return;
    char str[depth * 3];
    int curr = 0;
    for (int i = 0; i < depth; i++) {
        str[curr++] = getOrientationChar(moves[i].orientation);
        str[curr++] = getDirectionChar(moves[i].direction);
        str[curr++] = ' ';
    }
    str[curr++] = '\0';

    drawTextBoxed(str, FONT_SEMIBOLD, 80, y);
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
                Vector2 textWidth = MeasureTextEx(fontBold, num, 30, 3);
                int textX         = squareX + (squareSize - textWidth.x) / 2;
                int textY         = squareY + (squareSize - 20) / 2;
                DrawTextEx(fontBold, num, (Vector2){ textX, textY }, 30, 3, BLACK);
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

    int codepointCount;
    int *codepoints = LoadCodepoints(
        " !\"#$%&'()*+,-./0123456789:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
        &codepointCount);

    fontBold = fontGet(FONT_BOLD, 50);
    font     = fontGet(FONT_REGULAR, 100);

    resetColors();
    initFacePos();

    int W = GetScreenWidth(), H = GetScreenHeight();
    int margin    = 50;
    int barH      = 150;
    int barY      = H - barH - 2 * margin;
    int paletteY  = barY - squareSize - margin;
    int schemaEnd = facesPos[5].y + squareSize * 3;
    int textY     = schemaEnd + (paletteY - schemaEnd) / 2;  // paletteY - 120;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));

        if (drawButton(W - 200, 50, 150, 100, GetColor(0x666666FF), "DEBUG", fontBold)) debug = !debug;

        if (cameraIsActive()) {
            if (drawButton(margin, margin, 150, 100, GetColor(0x666666FF), "BACK", fontBold)) cameraClose();
            if (cameraUpdate()) {
                Texture2D tex = cameraGetTexture();
                float scale   = (float)W / tex.width;
                DrawTextureEx(tex, (Vector2){ 0, START_Y }, 0, scale, WHITE);
                float w     = tex.width * scale - 400;
                float cell  = w / 3;
                int start_x = 200, start_y = START_Y + 200;
                int line_width = 8;
                DrawRectangleLinesEx((Rectangle){ 200, START_Y + 200, w, w }, line_width, BLACK);
                DrawLineEx((Vector2){ start_x, start_y + w / 3 }, (Vector2){ start_x + w, start_y + w / 3 }, line_width,
                           BLACK);
                DrawLineEx((Vector2){ start_x, start_y + 2 * w / 3 }, (Vector2){ start_x + w, start_y + 2 * w / 3 },
                           line_width, BLACK);
                DrawLineEx((Vector2){ start_x + w / 3, start_y }, (Vector2){ start_x + w / 3, start_y + w }, line_width,
                           BLACK);
                DrawLineEx((Vector2){ start_x + 2 * w / 3, start_y }, (Vector2){ start_x + 2 * w / 3, start_y + w },
                           line_width, BLACK);

                if (screenWhite) {
                    screenWhite--;
                    DrawRectangle(0, START_Y, tex.width * scale, tex.height * scale,
                                  (Color){ WHITE.r, WHITE.g, WHITE.b, (255 / SCREEN_WHITE_FRAMES) * screenWhite });
                }

                char *names      = "ULFRDB";
                const char *text = TextFormat("Show face %c", names[scanFace]);
                int text_x       = (W - MeasureTextEx(fontBold, text, 75, 4).x) / 2;
                fontDraw(text, text_x, START_Y - 80, FONT_REGULAR, 80, RAYWHITE);

                Color scanned[9];
                for (int col = 0; col < 3; col++) {
                    for (int row = 0; row < 3; row++) {
                        float cx = start_x + (col + 0.5f) * cell;
                        float cy = start_y + (row + 0.5f) * cell;

                        int tx = cx / scale;
                        int ty = (cy - START_Y) / scale;

                        Color raw              = cameraSampleColor(tx, ty, (cell / scale) / 6);
                        scanned[col * 3 + row] = classifyColor(raw);

                        DrawRectangle(cx - 25, cy - 25, 50, 50, scanned[col * 3 + row]);
                        DrawRectangleLines(cx - 25, cy - 25, 50, 50, BLACK);

                        if (debug) {
                            Vector3 hsv   = ColorToHSV(raw);
                            const char *t = TextFormat("H %.0f  S %.2f  V %.2f", hsv.x, hsv.y, hsv.z);
                            int text_y    = (W - fontMeasure(t, FONT_REGULAR, 50)) / 2;
                            fontDraw(t, text_y, start_y + w + (col * 3 + row) * 70, FONT_REGULAR, 50, RAYWHITE);
                        }
                    }
                }

                if (drawButtonPro(W / 2 - 150, 2600, 300, 125, GetColor(0x9370DBFF), "SCAN", font, 100, BLACK, 0.55,
                                  1.f)) {
                    VibrateMS(75);
                    screenWhite = SCREEN_WHITE_FRAMES;
                    for (int k = 0; k < 9; k++) faces[scanFace][k] = scanned[k];
                    scanFace = (scanFace + 1) % 6;
                    if (scanFace == 0) cameraClose();  // all 6 faces scanned
                }
            }

            EndDrawing();
            continue;
        }

        if (drawButton(margin, margin, 150, 100, GetColor(0x666666FF), "RESET", fontBold)) {
            resetColors();
            depth        = 0;
            good         = false;
            selectedFace = -1;
        }

        if (selectedFace != -1) {
            int x = W / 2. - squareSize * 3 - faceMargin * 2.5;
            for (int i = 0; i < 6; i++) {
                if (drawButton(x, paletteY, squareSize, squareSize, colors[i], "", fontBold))
                    faces[selectedFace][selectedFacelet.x * 3 + selectedFacelet.y] = colors[i];
                x += squareSize + faceMargin;
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int mouse_x = GetMouseX(), mouse_y = GetMouseY();
            int face = getFaceTouched(mouse_x, mouse_y);
            if (face != -1) {
                selectedFace      = face;
                selectedFacelet.x = (mouse_x - facesPos[selectedFace].x) / squareSize;
                selectedFacelet.y = (mouse_y - facesPos[selectedFace].y) / squareSize;
            } else {
                selectedFace    = -1;
                selectedFacelet = (GridCoord){ -1, -1 };
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

        int half = (W - 3 * margin) / 2;

        if (drawButtonPro(margin, barY, half, barH, GetColor(0x666666FF), "CAMERA", font, 100, BLACK, 0.55, 1.f)) {
            scanFace = 0;
            cameraOpen();
        }
        if (drawButtonPro(2 * margin + half, barY, half, barH, GetColor(0x9370DBFF), "SOLVE", font, 100, BLACK, 0.55,
                          1.f)) {
            toStr(str);
            err  = findSolutionBasic(str, 25, 5000, moves, &depth);
            good = true;
        }
        if (err) {
            char *errMsg      = printErrorMessage(err);
            Vector2 textWidth = MeasureTextEx(fontBold, errMsg, 50, 4);
            DrawTextEx(fontBold, errMsg, (Vector2){ (W - textWidth.x) / 2, textY }, 50, 4, RED);
        } else if (!err && good)
            printMoves(textY);

        if (debug) {
            DrawLine(W / 2, 0, W / 2, GetScreenHeight(), BLACK);
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                DrawTextEx(fontBold, TextFormat("DOWN %d %d", GetMouseX(), GetMouseY()), (Vector2){ 100, 200 }, 50, 5,
                           RAYWHITE);
                DrawTextEx(fontBold, TextFormat("Selected face %d", selectedFace), (Vector2){ 100, 250 }, 50, 5,
                           RAYWHITE);
            }
        }

        EndDrawing();
    }
    cameraClose();
    UnloadFont(fontBold);
    UnloadFont(font);
    UnloadCodepoints(codepoints);
    fontShutdown();

    CloseWindow();

    return 0;
}
