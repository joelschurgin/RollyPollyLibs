#include "graphics.h"
#include "xdg-shell-protocol.c"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Arena* graphics_arena = 0L;
b32 graphics_intitiallized = 0;

#include "graphics_internal.c"
#include "graphics_platform_internal.c"
#include "graphics_font.c"

void graphics_init() {
    graphics_arena = default_arena();
    Assert(graphics_arena);

    // load opengl funcs
    {
        #define X(name, r, p) name = (name##_FunctionType *)_graphics_load_opengl_func(#name);
            Graphics_OpenGL_ProcedureXList
        #undef X
    }
}

Graphics_Window* graphics_window_create() {
    if (!graphics_intitiallized) graphics_init();

    Graphics_Window* window = push_struct(graphics_arena, Graphics_Window);
    window->width = 1280;
    window->height = 720;

    window->wl_display = wl_display_connect(NULL);
    Assert(window->wl_display);

    struct wl_registry* reg = wl_display_get_registry(window->wl_display);
    Assert(reg);

    wl_registry_add_listener(reg, &_graphics_registry_listener, window);
    wl_display_roundtrip(window->wl_display);

    window->egl_disp = eglGetDisplay(window->wl_display);
    Assert(window->egl_disp != EGL_NO_DISPLAY);
 
    EGLint major, minor;
    eglInitialize(window->egl_disp, &major, &minor);
    Assert(major == 1 && minor <= 5);

    eglBindAPI(EGL_OPENGL_API);

    EGLint attribs[] = {
        EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
        EGL_CONFORMANT,        EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

        EGL_RED_SIZE,      8,
        EGL_GREEN_SIZE,    8,
        EGL_BLUE_SIZE,     8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE,  8,

        EGL_NONE
    };
    EGLint num_configs;
    eglChooseConfig(window->egl_disp, attribs, &window->egl_cfg, 1, &num_configs);

    EGLint ctx_attribs[] = { 
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    window->egl_ctx = eglCreateContext(window->egl_disp, window->egl_cfg, EGL_NO_CONTEXT, ctx_attribs);

    window->surface = wl_compositor_create_surface(window->compositor);

    window->x_surface = xdg_wm_base_get_xdg_surface(window->shell, window->surface);
    xdg_surface_add_listener(window->x_surface, &_graphics_x_surface_listener, window);

    window->top = xdg_surface_get_toplevel(window->x_surface);
    xdg_toplevel_add_listener(window->top, &_graphics_top_listener, window);
    wl_surface_commit(window->surface);

    eglMakeCurrent(window->egl_disp, window->egl_surf, window->egl_surf, window->egl_ctx);

    {
        glViewport(0, 0, window->width, window->height);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        window->rect = graphics_rect_create();
        window->image_rect = graphics_image_rect_create("Graphics/Fonts/RobotoMono.png");
    }

    return window;
}

void graphics_window_close(Graphics_Window* window) {
    if (window->egl_disp) {
        eglMakeCurrent(window->egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (window->egl_surf) eglDestroySurface(window->egl_disp, window->egl_surf);
        if (window->egl_window) wl_egl_window_destroy(window->egl_window);
        if (window->egl_ctx) eglDestroyContext(window->egl_disp, window->egl_ctx);
        eglTerminate(window->egl_disp);
    }

    xdg_toplevel_destroy(window->top);
    xdg_surface_destroy(window->x_surface);
    wl_surface_destroy(window->surface);
    wl_display_disconnect(window->wl_display);
}

b32 graphics_poll_events(Graphics_Window* window) {
    return wl_display_dispatch(window->wl_display) && !window->closed;
}

void graphics_window_dimensions(Graphics_Window* window, u32* width, u32* height) {
    *width = window->width;
    *height = window->height;
}

void graphics_set_draw_callback(Graphics_Window* window, graphics_draw_func_t draw_func, void* data) {
    window->draw_func = draw_func;
    window->draw_func_data = data;
}

Graphics_Rect graphics_rect_create() {
    Graphics_Rect rect = {0};

    // verts
    {
        local_persist const f32 rect_verts[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
        };

        glGenVertexArrays(1, &rect.vao);
        glBindVertexArray(rect.vao);

        glGenBuffers(1, &rect.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, rect.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rect_verts), (void*)rect_verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
    }

    // shader
    TempArenaBlock(graphics_arena, temp_arena) {
        String vert_shader = _graphics_shader_read(temp_arena, String("Graphics/Shaders/box_vert.glsl"));
        String frag_shader = _graphics_shader_read(temp_arena, String("Graphics/Shaders/box_frag.glsl"));

        rect.shader = _graphics_shader_create(vert_shader, frag_shader);
    }

    // uniforms
    {
        rect.u_fill_color = glGetUniformLocation(rect.shader.program, "u_fill_color");
        rect.u_transform = glGetUniformLocation(rect.shader.program, "u_transform");
        rect.u_rect = glGetUniformLocation(rect.shader.program, "u_rect");
        rect.u_radius = glGetUniformLocation(rect.shader.program, "u_radius");
        rect.u_border_thickness = glGetUniformLocation(rect.shader.program, "u_border_thickness");
        rect.u_border_color = glGetUniformLocation(rect.shader.program, "u_border_color");
    }

    return rect;
}

void graphics_rect_fill(Graphics_Window* window, f32 rect_x, f32 rect_y, f32 rect_w, f32 rect_h, f32 radius, f32 border_thickness, Graphics_Color fill_color, Graphics_Color border_color) {
    glUseProgram(window->rect.shader.program);
    glUniform4f(window->rect.u_fill_color, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    glUniform4f(window->rect.u_border_color, border_color.r, border_color.g, border_color.b, border_color.a);

    const f32 padding = 8.0f;

    f32 w = 2.0f * (rect_w + padding * 2.0f) / window->width;
    f32 h = -2.0f * (rect_h + padding * 2.0f) / window->height;
    f32 x = 2.0f * (rect_x - padding) / window->width - 1.0f;
    f32 y = 1.0f - 2.0f * (rect_y - padding) / window->height;
    f32 matrix[] = {
        w, 0, 0, 0,
        0, h, 0, 0,
        0, 0, 1, 0,
        x, y, 0, 1,
    };

    glUniformMatrix4fv(window->rect.u_transform, 1, false, matrix);
    glUniform4f(window->rect.u_rect, rect_x, rect_y, rect_w, rect_h);
    glUniform1f(window->rect.u_radius, radius);

    glUniform1f(window->rect.u_border_thickness, border_thickness);

    glBindVertexArray(window->rect.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

Graphics_ImageRect graphics_image_rect_create(const char* path) {
    Graphics_ImageRect rect = {0};

    // verts
    {
        local_persist const float rect_verts[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
        };

        glGenVertexArrays(1, &rect.vao);
        glBindVertexArray(rect.vao);

        glGenBuffers(1, &rect.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, rect.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rect_verts), (void*)rect_verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
    }

    // texture and image loading
    {
        glGenTextures(1, &rect.texture_id);
        glBindTexture(GL_TEXTURE_2D, rect.texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        i32 img_w = 0;
        i32 img_h = 0;
        i32 num_channels = 0;

        u8* data = stbi_load(path, &img_w, &img_h, &num_channels, 0);
        if (!data) {
            printf("Error loading image!!\n");
            return rect;
        }
 
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GLenum internal_format = (num_channels == 4) ? GL_RGBA8 : GL_RGB8;
        GLenum format = (num_channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, img_w, img_h, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    // shader
    TempArenaBlock(graphics_arena, temp_arena) {
        String vert_shader = _graphics_shader_read(temp_arena, String("Graphics/Shaders/text_vert.glsl"));
        String frag_shader = _graphics_shader_read(temp_arena, String("Graphics/Shaders/text_frag.glsl"));

        rect.shader = _graphics_shader_create(vert_shader, frag_shader);
    }

    // uniforms
    {
        rect.u_transform = glGetUniformLocation(rect.shader.program, "u_transform");
    }

    return rect;
}

void graphics_image_rect_draw(Graphics_Window* window, f32 rect_x, f32 rect_y, f32 rect_w, f32 rect_h) {
    glUseProgram(window->image_rect.shader.program);

    f32 w = 2.0f * rect_w / window->width;
    f32 h = -2.0f * rect_h / window->height;
    f32 x = 2.0f * rect_x / window->width - 1.0f;
    f32 y = 1.0f - 2.0f * rect_y / window->height;
    f32 matrix[] = {
        w, 0, 0, 0,
        0, h, 0, 0,
        0, 0, 1, 0,
        x, y, 0, 1,
    };

    glUniformMatrix4fv(window->image_rect.u_transform, 1, false, matrix);

    glBindTexture(GL_TEXTURE_2D, window->image_rect.texture_id);
    glBindVertexArray(window->image_rect.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
