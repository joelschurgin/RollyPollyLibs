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

internal void graphics_font_load_atlas(String font_family, Graphics_Font* font) {
    TempArenaBlock(graphics_arena) {
        String path = string_concat(graphics_arena, font_family, String(".json"));
        FileBlock(graphics_arena, path, FILE_READ_ONLY, f) {
            String file_str = (String){
                .str = (u8*)f->data,
                .size = file_size(f),
            };
            _graphics_font_parse(file_str, font);
        }
    }
}

internal void graphics_font_load_texture(String font_family, Graphics_Font* font) {
    Assert(font && font->type == GRAPHICS_FONT_TYPE_MSDF);

    // verts
    {
        local_persist const float rect_verts[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
        };

        glGenVertexArrays(1, &font->vao);
        glBindVertexArray(font->vao);

        glGenBuffers(1, &font->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, font->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rect_verts), (void*)rect_verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
    }

    TempArenaBlock(graphics_arena) {
        String path = string_concat(graphics_arena, font_family, String(".png\0"));

        // texture and image loading
        glGenTextures(1, &font->texture_id);
        glBindTexture(GL_TEXTURE_2D, font->texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        i32 img_w = 0;
        i32 img_h = 0;
        i32 num_channels = 0;

        u8* data = stbi_load(path.str, &img_w, &img_h, &num_channels, 0);
        if (!data) {
            printf("Error loading image!!\n");
            return;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GLenum internal_format = (num_channels == 4) ? GL_RGBA8 : GL_RGB8;
        GLenum format = (num_channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, img_w, img_h, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    // Texture Buffer Object
    TempArenaBlock(graphics_arena) {
        Graphics_FontGlyphBoundsArray glyph_bounds = Array(graphics_arena, Graphics_FontGlyphBounds, GRAPHICS_FONT_NUM_GLYPHS);
        for (u64 i = 0; i < GRAPHICS_FONT_NUM_GLYPHS; i++) {
            glyph_bounds.data[i] = (Graphics_FontGlyphBounds){
                .atlas_left   = font->glyphs[i].atlas_left   / font->width,
                .atlas_bottom = font->glyphs[i].atlas_bottom / font->height,
                .atlas_right  = font->glyphs[i].atlas_right  / font->width,
                .atlas_top    = font->glyphs[i].atlas_top    / font->height,

                .plane_left   = font->glyphs[i].plane_left,
                .plane_bottom = font->glyphs[i].plane_bottom,
                .plane_right  = font->glyphs[i].plane_right,
                .plane_top    = font->glyphs[i].plane_top,
            };
        }

        glGenBuffers(1, &font->tbo_font_library);
        glBindBuffer(GL_TEXTURE_BUFFER, font->tbo_font_library);
        glBufferData(GL_TEXTURE_BUFFER, glyph_bounds.count * sizeof(Graphics_FontGlyphBounds), glyph_bounds.data, GL_STATIC_DRAW);

        glGenTextures(1, &font->tbo_font_library_texture);
        glBindTexture(GL_TEXTURE_BUFFER, font->tbo_font_library_texture);

        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, font->tbo_font_library);

        glGenBuffers(1, &font->tbo_string_buffer);
        glBindBuffer(GL_TEXTURE_BUFFER, font->tbo_string_buffer);
        //glBufferData(GL_TEXTURE_BUFFER, glyph_bounds.count * sizeof(Graphics_FontGlyphBounds), glyph_bounds.data, GL_STATIC_DRAW);

        glGenTextures(1, &font->tbo_string_buffer_texture);
        glBindTexture(GL_TEXTURE_BUFFER, font->tbo_string_buffer_texture);

        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, font->tbo_string_buffer);
    }

    // shader - could load this once for all fonts?
    TempArenaBlock(graphics_arena) {
        String vert_shader = _graphics_shader_read(graphics_arena, String("Graphics/Shaders/text_vert.glsl"));
        String frag_shader = _graphics_shader_read(graphics_arena, String("Graphics/Shaders/text_frag.glsl"));

        font->shader = _graphics_shader_create(vert_shader, frag_shader);
    }

    // uniforms
    {
        font->u_transform = glGetUniformLocation(font->shader.program, "u_transform");
        font->u_texture = glGetUniformLocation(font->shader.program, "u_texture");
        font->u_fontLibrary = glGetUniformLocation(font->shader.program, "u_fontLibrary");
        font->u_stringBuffer = glGetUniformLocation(font->shader.program, "u_stringBuffer");
        font->u_pxRange = glGetUniformLocation(font->shader.program, "u_pxRange");
        font->u_fontSize = glGetUniformLocation(font->shader.program, "u_fontSize");
        font->u_pos = glGetUniformLocation(font->shader.program, "u_pos");
    }
}

Graphics_Font* graphics_font_load(String font_family) {
    Graphics_Font* font = push_struct(graphics_arena, Graphics_Font);

    graphics_font_load_atlas(font_family, font);
    graphics_font_load_texture(font_family, font);

    return font;
}

void graphics_font_draw(Graphics_Window* window, Graphics_Font* font, f32 x, f32 y, f32 font_size) {
    glUseProgram(font->shader.program);
 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texture_id);
    glUniform1i(font->u_texture, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, font->tbo_font_library_texture);
    glUniform1i(font->u_fontLibrary, 1);

    glUniform1f(font->u_pxRange, font->distance_range);
    glUniform1f(font->u_fontSize, font_size);
    glUniform2f(font->u_pos, x, y);

    f32 w = 2.0f / window->width;
    f32 h = 2.0f / window->height;
    f32 matrix[] = {
         w,  0,  0,  0,
         0,  h,  0,  0,
         0,  0,  1,  0,
        -1, -1,  0,  1,
    };


    glUniformMatrix4fv(font->u_transform, 1, false, matrix);

    glBindVertexArray(font->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
    glBindVertexArray(0);
}
