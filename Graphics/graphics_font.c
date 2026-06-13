internal String _graphics_font_parse_read_string(u8** c_ptr, String file_str) {
    String token;
    u8* c;

    DeferBlock({ c = *c_ptr; }, { *c_ptr = ++c; }) {
        token.str = ++c;
        for EachCharContinueUntil(c, file_str, *c == '"');
        token.size = (u64)(c - token.str);
    }

    return token;
}

internal f32 _graphics_font_parse_read_f32(u8** c_ptr) {
    return strtof(*c_ptr, (char**)c_ptr);
}

internal u8 _graphics_font_parse_read_u8(u8** c_ptr) {
    return (u8)_graphics_font_parse_read_f32(c_ptr);
}

internal u8* _graphics_font_parse_atlas(u8* c, String file_str, Graphics_Font* font) {
    for EachCharContinueUntil(c, file_str, *c == '{');

    for EachCharContinueUntil(c, file_str, *c == '}') {
        for EachCharContinueUntil(c, file_str, *c == '"');
        String token = _graphics_font_parse_read_string(&c, file_str);
        for EachCharContinueUntil(c, file_str, *c == ':');
        c++;

        if (string_compare(token, String("type"))) {
            for EachCharContinueUntil(c, file_str, *c == '"');
            String val = _graphics_font_parse_read_string(&c, file_str);
            Assert(string_compare(val, String("msdf")));
            font->type = GRAPHICS_FONT_TYPE_MSDF;
        } else if (string_compare(token, String("distanceRange"))) {
            font->distance_range = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("distanceRangeMiddle"))) {
            font->distance_range_middle = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("size"))) {
            font->atlas_size = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("width"))) {
            font->width = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("height"))) {
            font->height = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("yOrigin"))) {
            for EachCharContinueUntil(c, file_str, *c == '"');
            String val = _graphics_font_parse_read_string(&c, file_str);
            if (string_compare(val, String("top"))) {
                font->y_origin = GRAPHICS_FONT_Y_ORIGIN_TOP;
            } else if (string_compare(val, String("bottom"))) {
                font->y_origin = GRAPHICS_FONT_Y_ORIGIN_BOTTOM;
            } else {
                AssertAlways(!"Unknown y origin!!!");
            }
        } else {
            Assert(!"Unsupported font atlas field!!");
        }

        for EachCharContinueUntil(c, file_str, *c == ',' || *c == '}');
        c--;
    }

    return c;
}

internal u8* _graphics_font_parse_metrics(u8* c, String file_str, Graphics_Font* font) {
    for EachCharContinueUntil(c, file_str, *c == '{');

    for EachCharContinueUntil(c, file_str, *c == '}') {
        for EachCharContinueUntil(c, file_str, *c == '"');
        String token = _graphics_font_parse_read_string(&c, file_str);
        for EachCharContinueUntil(c, file_str, *c == ':');
        c++;

        if (string_compare(token, String("emSize"))) {
            font->em_size = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("lineHeight"))) {
            font->line_height = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("ascender"))) {
            font->ascender = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("descender"))) {
            font->descender =  _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("underlineY"))) {
            font->underline_y = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("underlineThickness"))) {
            font->underline_thickness = _graphics_font_parse_read_f32(&c);
        } else {
            Assert(!"Unsupported font metrics field!!");
        }

        for EachCharContinueUntil(c, file_str, *c == ',' || *c == '}');
        c--;
    }

    return c;
}

internal void _graphics_font_parse_glyphs_single_bounds(String data, f32* left, f32* right, f32* bottom, f32* top) {
    for EachChar(c, data) {
        for EachCharContinueUntil(c, data, *c == '"');
        String token = _graphics_font_parse_read_string(&c, data);
        for EachCharContinueUntil(c, data, *c == ':');
        c++;

        if (string_compare(token, String("left"))) {
            *left = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("right"))) {
            *right = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("bottom"))) {
            *bottom = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("top"))) {
            *top = _graphics_font_parse_read_f32(&c);
        } else {
            Assert(!"Unsupported font glyph single field!!");
        }
    }
}

internal void _graphics_font_parse_glyphs_single(String glyph_data, Graphics_Font* font) {
    Graphics_FontGlyph g = {0};

    for EachChar(c, glyph_data) {
        for EachCharContinueUntil(c, glyph_data, *c == '"');
        String token = _graphics_font_parse_read_string(&c, glyph_data);
        for EachCharContinueUntil(c, glyph_data, *c == ':');
        c++;

        if (string_compare(token, String("unicode"))) {
            g.unicode = _graphics_font_parse_read_u8(&c);
        } else if (string_compare(token, String("advance"))) {
            g.advance = _graphics_font_parse_read_f32(&c);
        } else if (string_compare(token, String("planeBounds"))) {
            String data;

            for EachCharContinueUntil(c, glyph_data, *c == '{');
            data.str = ++c;
            for EachCharContinueUntil(c, glyph_data, *c == '}');
            data.size = (u64)(c - data.str);

            _graphics_font_parse_glyphs_single_bounds(data,
                                                    &g.plane_left,
                                                    &g.plane_right,
                                                    &g.plane_bottom,
                                                    &g.plane_top);
        } else if (string_compare(token, String("atlasBounds"))) {
            String data;

            for EachCharContinueUntil(c, glyph_data, *c == '{');
            data.str = ++c;
            for EachCharContinueUntil(c, glyph_data, *c == '}');
            data.size = (u64)(c - data.str);

            _graphics_font_parse_glyphs_single_bounds(data,
                                                    &g.atlas_left,
                                                    &g.atlas_right,
                                                    &g.atlas_bottom,
                                                    &g.atlas_top);
        } else {
            Assert(!"Unsupported font glyph single field!!");
        }
    }

    font->glyphs[g.unicode] = g;
}

internal u8* _graphics_font_parse_glyphs(u8* c, String file_str, Graphics_Font* font) {
    for EachCharContinueUntil(c, file_str, *c == '[');

    for EachCharContinueUntil(c, file_str, *c == ']') {
        String glyph_data;

        i32 num_brackets = 1;
        for EachCharContinueUntil(c, file_str, *c == '{');
        glyph_data.str = ++c;
        for EachCharContinue(c, file_str) {
            num_brackets += (*c == '{') - (*c == '}');
            if (num_brackets <= 0) break;
        }
        glyph_data.size = (u64)(c - glyph_data.str);

        _graphics_font_parse_glyphs_single(glyph_data, font);
    }

    return c;
}

internal void _graphics_font_parse(String file_str, Graphics_Font* font) {
    for EachChar(c, file_str) {
        if (*c == '"') {
            String token = _graphics_font_parse_read_string(&c, file_str);
            if (string_compare(token, String("atlas"))) {
                c = _graphics_font_parse_atlas(c, file_str, font);
            } else if (string_compare(token, String("metrics"))) {
                c = _graphics_font_parse_metrics(c, file_str, font);
            } else if (string_compare(token, String("glyphs"))) {
                c = _graphics_font_parse_glyphs(c, file_str, font);
            }
        }
    }
}

Graphics_Font* graphics_font_load(String font_family) {
    Graphics_Font* font = push_struct(graphics_arena, Graphics_Font);

    TempArenaBlock(graphics_arena, temp_arena) {
        String path = string_concat(temp_arena, font_family, String(".json"));
        File f;
        FileBlock(temp_arena, path, FILE_READ_ONLY, f) {
            String file_str = (String){
                .str = (u8*)f.data,
                .size = file_size(&f),
            };
            _graphics_font_parse(file_str, font);
        }
    }

    return font;
}
