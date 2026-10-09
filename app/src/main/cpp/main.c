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

#define FACE_SIZE (3 * squareSize + 2 * squareMargin)

#define START_X (GetScreenWidth() / 2. - FACE_SIZE / 2.)
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

int squareSize   = 135;
int squareMargin = 10;
int faceMargin   = 35;

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
    float squareRoundness = 0.3f;
    const int pad         = 8;

    float innerRadius = squareRoundness * squareSize / 2.f;
    float bgSize      = FACE_SIZE + 2 * pad;
    float bgRoundness = 2.f * (innerRadius + pad) / bgSize;

    DrawRectangleRounded((Rectangle){ x - pad, y - pad, bgSize, bgSize }, bgRoundness, 0, GRAY);

    for (int j = 0; j < 3; j++) {
        int squareY = y + j * (squareSize + squareMargin);
        for (int i = 0; i < 3; i++) {
            int squareX = x + i * (squareSize + squareMargin);
            Rectangle r = { squareX, squareY, squareSize, squareSize };
            DrawRectangleRounded(r, squareRoundness, 1, faces[index][i * 3 + j]);
            DrawRectangleRoundedLines(r, squareRoundness, 1, BLACK);

            if (i == 1 && j == 1) {
                const char *t = TextFormat("%c", getNotationFromIndex(index));
                int tw        = fontMeasure(t, FONT_SEMIBOLD, 50);
                int ty        = fontMeasureY(FONT_SEMIBOLD, 50);
                fontDraw(t, squareX + (squareSize - tw) / 2, squareY + (squareSize - ty) / 2, FONT_SEMIBOLD, 50, BLACK);
            }

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
    int gridStartX = GetScreenWidth() / 2. - (FACE_SIZE * 1.5 + faceMargin);
    int spacing    = FACE_SIZE + faceMargin;

    int offsetX = px - gridStartX;
    int offsetY = py - START_Y;

    if (offsetX < 0 || offsetY < 0) return -1;

    // Check if click is in a margin between squares
    int marginX = offsetX % spacing;
    int marginY = offsetY % spacing;

    if (marginX >= FACE_SIZE || marginY >= FACE_SIZE) return -1;

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
    facesPos[1] = (GridCoord){ START_X - FACE_SIZE - faceMargin, START_Y + FACE_SIZE + faceMargin };
    facesPos[2] = (GridCoord){ START_X, START_Y + FACE_SIZE + faceMargin };
    facesPos[3] = (GridCoord){ START_X + FACE_SIZE + faceMargin, START_Y + FACE_SIZE + faceMargin };
    facesPos[4] = (GridCoord){ START_X, START_Y + 2 * (FACE_SIZE + faceMargin) };
    facesPos[5] = (GridCoord){ START_X, START_Y + 3 * (FACE_SIZE + faceMargin) };
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

    Texture2D bugIcon    = loadIcon("bug.png", true);
    Texture2D undoIcon   = loadIcon("undo.png", true);
    Texture2D cameraIcon = loadIcon("camera.png", true);
    Texture2D backIcon   = loadIcon("back.png", true);

    resetColors();
    initFacePos();

    int W = GetScreenWidth(), H = GetScreenHeight();
    int margin    = 50;
    int barH      = 150;
    int barY      = H - barH - 2 * margin;
    int paletteY  = barY - squareSize - margin;
    int schemaEnd = facesPos[5].y + FACE_SIZE;
    int textY     = schemaEnd + (paletteY - schemaEnd) / 2;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x1a1a1aFF));

        // Light gray when off, purple when debug is on
        Color debugColor = debug ? GetColor(0x9370DBFF) : GetColor(0xCCCCCCFF);
        int debugSize    = 125;
        if (drawIconButton(W - debugSize - margin, margin, debugSize, bugIcon, debugColor)) debug = !debug;

        if (cameraIsActive()) {
            if (drawIconButton(margin, margin, 150, backIcon, debugColor)) cameraClose();
            if (cameraUpdate()) {
                Texture2D tex = cameraGetTexture();
                float scale   = (float)W / tex.width;
                DrawTextureEx(tex, (Vector2){ 0, START_Y }, 0, scale, WHITE);
                float w     = tex.width * scale - 400;
                float cell  = w / 3;
                int start_x = 200, start_y = START_Y + 200;
                int line_width = 8;
                int gap        = 20;
                float round    = 0.3f;

                for (int col = 0; col < 3; col++) {
                    for (int row = 0; row < 3; row++) {
                        Rectangle r = { start_x + col * cell + gap / 2.f, start_y + row * cell + gap / 2.f, cell - gap,
                                        cell - gap };
                        DrawRectangleRoundedLinesEx(r, round, 0, line_width, BLACK);
                    }
                }

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

                        Rectangle swatch = { cx - 25, cy - 25, 50, 50 };
                        DrawRectangleRounded(swatch, round, 0, scanned[col * 3 + row]);
                        DrawRectangleRoundedLinesEx(swatch, round, 0, 3, BLACK);

                        if (debug) {
                            Vector3 hsv   = ColorToHSV(raw);
                            const char *t = TextFormat("H %.0f  S %.2f  V %.2f", hsv.x, hsv.y, hsv.z);
                            int text_y    = (W - fontMeasure(t, FONT_REGULAR, 50)) / 2;
                            fontDraw(t, text_y, start_y + w + (col * 3 + row) * 70, FONT_REGULAR, 50, RAYWHITE);
                        }
                    }
                }

                if (drawIconTextButton(W / 2 - W / 6, 2600, W / 3, 125, GetColor(0x00E0C7FF), "SCAN", font, 100,
                                       GetColor(0x00E0C7FF), cameraIcon, 0.55)) {
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

        if (drawIconButton(margin, margin, debugSize, undoIcon, debugColor)) {
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
                int relX          = mouse_x - facesPos[selectedFace].x;
                int relY          = mouse_y - facesPos[selectedFace].y;
                selectedFacelet.x = relX / (squareSize + squareMargin);
                selectedFacelet.y = relY / (squareSize + squareMargin);
            } else {
                selectedFace    = -1;
                selectedFacelet = (GridCoord){ -1, -1 };
            }
        }

        for (int i = 0; i < 6; i++) {
            int x = facesPos[i].x, y = facesPos[i].y;
            draw3x3(x, y, i);
            if (selectedFace == i) {
                int hx = facesPos[i].x + selectedFacelet.x * (squareSize + squareMargin);
                int hy = facesPos[i].y + selectedFacelet.y * (squareSize + squareMargin);
                DrawRectangleRoundedLinesEx((Rectangle){ hx, hy, squareSize, squareSize }, 0.3, 1, 3, BLACK);
            }
        }

        int half = (W - 3 * margin) / 2;

        if (drawIconTextButton(margin, barY, half, barH, GetColor(0x00E0C7FF), "Camera", font, 100,
                               GetColor(0x00E0C7FF), cameraIcon, 0.55)) {
            scanFace = 0;
            cameraOpen();
        }
        if (drawButtonPro(2 * margin + half, barY, half, barH, GetColor(0x00E0C7FF), "SOLVE", font, 100, BLACK, 0.55,
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
    UnloadTexture(bugIcon);
    UnloadTexture(undoIcon);
    UnloadTexture(cameraIcon);
    UnloadTexture(backIcon);
    UnloadFont(fontBold);
    UnloadFont(font);
    UnloadCodepoints(codepoints);
    fontShutdown();

    CloseWindow();

    return 0;
}
