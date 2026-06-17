#ifndef GRAPHICS_TYPES_H
#define GRAPHICS_TYPES_H

extern Arena* graphics_arena;

typedef void (*graphics_draw_func_t)(void* data);

typedef struct {
    f32 r, g, b, a;
} Graphics_Color;
#define Graphics_Color(red, green, blue, alpha) (Graphics_Color){ .r = (red), .g = (green), .b = (blue), .a = (alpha) }

typedef struct {
    i32 program;
} Graphics_Shader;

typedef struct {
    Graphics_Shader shader;
    i32 vbo;
    i32 vao;
    i32 u_fill_color;
    i32 u_transform;
    i32 u_rect;
    i32 u_radius;
    i32 u_border_thickness;
    i32 u_border_color;
} Graphics_Rect;

typedef struct {
    Graphics_Shader shader;
    i32 vbo;
    i32 vao;
    i32 texture_id;
    i32 u_transform;
} Graphics_ImageRect;

typedef struct {
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

    Graphics_Rect rect;
    Graphics_ImageRect image_rect;
} Graphics_Window;

#endif
