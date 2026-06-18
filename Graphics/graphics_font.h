#ifndef GRAPHICS_FONT_H
#define GRAPHICS_FONT_H

#define GRAPHICS_FONT_NUM_GLYPHS 128

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
    f32 atlas_bottom;
    f32 atlas_right;
    f32 atlas_top;

    f32 plane_left;
    f32 plane_bottom;
    f32 plane_right;
    f32 plane_top;
} Graphics_FontGlyphBounds;

DefineArray(Graphics_FontGlyphBounds);

typedef struct {
    f32 atlas_left;
    f32 atlas_bottom;
    f32 atlas_right;
    f32 atlas_top;

    f32 plane_left;
    f32 plane_bottom;
    f32 plane_right;
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

    Graphics_FontGlyph glyphs[GRAPHICS_FONT_NUM_GLYPHS];

    Graphics_Shader shader;
    i32 vbo;
    i32 vao;

    i32 tbo_font_library;
    i32 tbo_font_library_texture;
    i32 u_fontLibrary;

    i32 tbo_string_buffer;
    i32 tbo_string_buffer_texture;
    i32 u_stringBuffer;

    i32 texture_id;
    i32 u_transform;
    i32 u_texture;

    i32 u_pxRange;
    i32 u_fontSize;
    i32 u_pos;

    // enums here for alignment
    Graphics_FontType type;
    Graphics_FontYOrigin y_origin;
} Graphics_Font;

Graphics_Font* graphics_font_load(String font_family);

void graphics_font_draw(Graphics_Window* window, Graphics_Font* font, f32 x, f32 y, f32 font_size);

#endif
