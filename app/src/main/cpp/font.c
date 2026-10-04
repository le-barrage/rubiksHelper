#include "font.h"

#include <stddef.h>

#define FONT_CACHE_MAX 16

static const char *FONT_PATHS[FONT_STYLE_COUNT] = {
    [FONT_LIGHT] = "SpaceGrotesk/SpaceGrotesk-Light.ttf",   [FONT_REGULAR] = "SpaceGrotesk/SpaceGrotesk-Regular.ttf",
    [FONT_MEDIUM] = "SpaceGrotesk/SpaceGrotesk-Medium.ttf", [FONT_SEMIBOLD] = "SpaceGrotesk/SpaceGrotesk-SemiBold.ttf",
    [FONT_BOLD] = "SpaceGrotesk/SpaceGrotesk-Bold.ttf",
};

typedef struct {
    FontStyle style;
    int size;
    Font font;
} cache_entry_t;

static cache_entry_t cache[FONT_CACHE_MAX];
static int cache_count;

static int spacingFor (int size)
{
    int s = size / 10;
    return s < 1 ? 1 : s;
}

static const Font *getFont (FontStyle style, int size)
{
    if (style < 0 || style > FONT_STYLE_COUNT) return NULL;

    for (int i = 0; i < cache_count; i++)
        if (cache[i].style == style && cache[i].size == size) return &cache[i].font;

    if (cache_count >= FONT_CACHE_MAX) return NULL;

    Font f = LoadFontEx(FONT_PATHS[style], size, NULL, 0);
    if (f.texture.id == 0) return NULL;

    cache[cache_count].style = style;
    cache[cache_count].size  = size;
    cache[cache_count].font  = f;
    return &cache[cache_count++].font;
}

void fontDraw (const char *text, int x, int y, FontStyle style, int size, Color color)
{
    const Font *f = getFont(style, size);
    if (f)
        DrawTextEx(*f, text, (Vector2){ (float)x, (float)y }, (float)size, (float)spacingFor(size), color);
    else
        DrawText(text, x, y, size, color);
}

int fontMeasure (const char *text, FontStyle style, int size)
{
    const Font *f = getFont(style, size);
    if (f) return (int)MeasureTextEx(*f, text, (float)size, (float)spacingFor(size)).x;
    return MeasureText(text, size);
}

int fontMeasureY (FontStyle style, int size)
{
    const Font *f = getFont(style, size);
    if (f) return (int)MeasureTextEx(*f, "O", (float)size, (float)spacingFor(size)).y;
    return size;
}

Font fontGet (FontStyle style, int size)
{
    const Font *f = getFont(style, size);
    return f ? *f : GetFontDefault();
}

void fontShutdown (void)
{
    for (int i = 0; i < cache_count; i++) UnloadFont(cache[i].font);
    cache_count = 0;
}
