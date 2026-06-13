#ifndef GRAPHICS_FONT_H
#define GRAPHICS_FONT_H

typedef enum {
    GRAPHICS_FONT_TYPE_NONE,
    GRAPHICS_FONT_TYPE_MSDF,
} Graphics_FontType;

typedef enum {
    GRAPHICS_FONT_Y_ORIGIN_TOP,
    GRAPHICS_FONT_Y_ORIGIN_BOTTOM,
} Graphics_FontYOrigin;

typedef struct {
    f32 atlas_left;
    f32 atlas_right;
    f32 atlas_bottom;
    f32 atlas_top;

    f32 plane_left;
    f32 plane_right;
    f32 plane_bottom;
    f32 plane_top;
 
    f32 advance;
    u8 unicode;
} Graphics_FontGlyph;

typedef struct {
    // atlas
    f32 distance_range;
    f32 distance_range_middle;
    f32 atlas_size;
    f32 width;
    f32 height;

    // metrics
    f32 em_size;
    f32 line_height;
    f32 ascender;
    f32 descender;
    f32 underline_y;
    f32 underline_thickness;

    Graphics_FontGlyph glyphs[128];

    Graphics_FontType type;
    Graphics_FontYOrigin y_origin;
} Graphics_Font;

Graphics_Font* graphics_font_load(String font_family);

#endif
