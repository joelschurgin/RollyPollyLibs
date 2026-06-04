#include "graphics.h"
#include "xdg-shell-protocol.c"

Arena* graphics_arena = 0L;
b32 graphics_intitiallized = 0;

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
};

#include "graphics_platform_internal.c"

void graphics_init() {
    graphics_arena = default_arena();
    Assert(graphics_arena);
}

Graphics_Window* graphics_window_create() {
    if (!graphics_intitiallized) graphics_init();

    Graphics_Window* window = push_struct(graphics_arena, Graphics_Window);

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

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    window->egl_ctx = eglCreateContext(window->egl_disp, window->egl_cfg, EGL_NO_CONTEXT, ctx_attribs);

    window->surface = wl_compositor_create_surface(window->compositor);

    window->x_surface = xdg_wm_base_get_xdg_surface(window->shell, window->surface);
    xdg_surface_add_listener(window->x_surface, &_graphics_x_surface_listener, window);

    window->top = xdg_surface_get_toplevel(window->x_surface);
    xdg_toplevel_add_listener(window->top, &_graphics_top_listener, window);
    wl_surface_commit(window->surface);

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

void graphics_set_draw_callback(Graphics_Window* window, graphics_draw_func_t draw_func, void* data) {
    window->draw_func = draw_func;
    window->draw_func_data = data;
}
