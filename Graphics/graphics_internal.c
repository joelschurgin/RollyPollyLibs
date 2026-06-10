struct Graphics_Shader {
    i32 program;
};

struct Graphics_Rect {
    struct Graphics_Shader shader;
    i32 vbo;
    i32 vao;
    i32 u_fill_color;
    i32 u_transform;
    i32 u_rect;
    i32 u_radius;
    i32 u_border_thickness;
    i32 u_border_color;
};

struct Graphics_ImageRect {
    struct Graphics_Shader shader;
    i32 vbo;
    i32 vao;
    i32 texture_id;
    i32 u_transform;
};

struct Graphics_Window {
#ifdef OS_LINUX
    struct wl_display* wl_display;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct xdg_wm_base* shell;
    struct xdg_toplevel* top;
    struct xdg_surface* x_surface;

    EGLDisplay egl_disp;
    EGLConfig egl_cfg;
    EGLContext egl_ctx;
    EGLSurface egl_surf;

    struct wl_egl_window* egl_window;
#endif

    graphics_draw_func_t draw_func;
    void* draw_func_data;

    u32 width;
    u32 height;

    b8 closed;

    struct Graphics_Rect rect;
    struct Graphics_ImageRect image_rect;
};

internal String _graphics_shader_read(Arena* arena, String path) {
    File f;
    String shader = String("");
    FileBlock(arena, path, FILE_READ_ONLY, f) {
        shader = EmptyString(arena, file_size(&f) + 1);
        shader.size--;
        file_read_bytes(&f, 0, shader.str, shader.size);
    }

    return shader;
}

internal u32 _graphics_shader_compile(u32 type, String source) {
    u32 shader = glCreateShader(type);
    Assert(shader > 0);

    glShaderSource(shader, 1, ((char**)&source.str), 0L);
    glCompileShader(shader);

    i32 status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        i32 info_log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);

        char info_log[info_log_length];
        glGetShaderInfoLog(shader, info_log_length, &info_log_length, info_log);
        printf("Shader Compilation Error: %s\n", info_log);
    }

    return shader;
}

internal Graphics_Shader _graphics_shader_create(String vert_shader, String frag_shader) {
    u32 vert_shader_id = _graphics_shader_compile(GL_VERTEX_SHADER, vert_shader);
    u32 frag_shader_id = _graphics_shader_compile(GL_FRAGMENT_SHADER, frag_shader);

    Graphics_Shader shader = (Graphics_Shader){0};
    shader.program = glCreateProgram();

    glAttachShader(shader.program, vert_shader_id);
    glAttachShader(shader.program, frag_shader_id);
    glLinkProgram(shader.program);

    i32 status = GL_FALSE;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        i32 info_log_length = 0;
        glGetProgramiv(shader.program, GL_INFO_LOG_LENGTH, &info_log_length);

        char info_log[info_log_length];
        glGetProgramInfoLog(shader.program, info_log_length, &info_log_length, info_log);
        printf("Shader Linking Error: %s\n", info_log);
        exit(1);
    }

    glDetachShader(shader.program, vert_shader_id);
    glDetachShader(shader.program, frag_shader_id);

    glDeleteShader(vert_shader_id);
    glDeleteShader(frag_shader_id);

    return shader;
}
