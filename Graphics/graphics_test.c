#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <wayland-egl.h>

#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define EGL_STATUS AssertAlways

struct wl_compositor* compositor = NULL;
struct wl_surface* surface = NULL;
struct xdg_wm_base* shell;
struct xdg_toplevel* top;

EGLDisplay egl_disp = NULL;
EGLConfig  egl_cfg  = NULL;
EGLContext egl_ctx  = NULL;
EGLSurface egl_surf = NULL;
struct wl_egl_window* egl_window = NULL; 

u16 w = 200;
u16 h = 100;

u8 brightness = 128;
b8 closed = 0;

void draw();


static void on_frame_done(void* data, struct wl_callback* callback, u32 callback_data) {
    wl_callback_destroy(callback);

    if (closed) {
        return;
    }

    draw();
}

static const struct wl_callback_listener frame_listener = {
    .done = on_frame_done
};

void resize() {
    if (!egl_window) {
        egl_window = wl_egl_window_create(surface, w, h);
        egl_surf = eglCreateWindowSurface(egl_disp, egl_cfg, egl_window, NULL);
        eglMakeCurrent(egl_disp, egl_surf, egl_surf, egl_ctx);
    } else {
        wl_egl_window_resize(egl_window, w, h, 0, 0);
    }
}

void draw() {
    float b = brightness / 255.0f;
    glClearColor(b, b, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // opengl draw calls here


    struct wl_callback* callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener, NULL);

    eglSwapBuffers(egl_disp, egl_surf);
}

void top_configure(void* data, struct xdg_toplevel* top, i32 new_width, i32 new_height, struct wl_array* stat) {
    if (!new_width && !new_height) {
        return;
    }

    if (w != new_width || h != new_height) {
        w = new_width;
        h = new_height;
        resize();
    }
}

void top_close(void* data, struct xdg_toplevel* top) {
    closed = 1;
}

struct xdg_toplevel_listener top_listener = {
    .configure = top_configure,
    .close = top_close,
};

void x_surface_conf(void* data, struct xdg_surface* x_surface, u32 serial) {
    xdg_surface_ack_configure(x_surface, serial);

    if (!egl_surf) {
        resize();
    }

    draw();
}

struct xdg_surface_listener x_surface_listener = {
    .configure = x_surface_conf,
};

void sh_ping(void* data, struct xdg_wm_base* sh, u32 serial) {
    xdg_wm_base_pong(sh, serial);
}

struct xdg_wm_base_listener sh_listener = {
    .ping = sh_ping,
};

void reg_glob(void* data, struct wl_registry* reg, u32 name, const char* interface, u32 version) {
    if(!strcmp(interface, wl_compositor_interface.name)) {
        compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
    } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        shell = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(shell, &sh_listener, 0);
    }
}

void reg_glob_remove(void* data, struct wl_registry* reg, u32 name) {

}

#undef global
struct wl_registry_listener reg_listener = {
    .global = reg_glob,
    .global_remove = reg_glob_remove,
};
#define global static

i32 main(i32 argc, u8 **argv) {
    struct wl_display* display = wl_display_connect(NULL);
    Assert(display);

    struct wl_registry* reg = wl_display_get_registry(display);
    Assert(reg);

    wl_registry_add_listener(reg, &reg_listener, 0);
    wl_display_roundtrip(display);

    egl_disp = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, display, NULL);
    Assert(egl_disp != EGL_NO_DISPLAY);
 
    EGLint major, minor;
    eglInitialize(egl_disp, &major, &minor);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint attribs[] = {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
    };
    EGLint num_configs;
    eglChooseConfig(egl_disp, attribs, &egl_cfg, 1, &num_configs);

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    egl_ctx = eglCreateContext(egl_disp, egl_cfg, EGL_NO_CONTEXT, ctx_attribs);

    surface = wl_compositor_create_surface(compositor);

    struct xdg_surface* x_surface = xdg_wm_base_get_xdg_surface(shell, surface);
    xdg_surface_add_listener(x_surface, &x_surface_listener, 0);

    top = xdg_surface_get_toplevel(x_surface);
    xdg_toplevel_add_listener(top, &top_listener, 0);
    xdg_toplevel_set_title(top, "Does this actually work?");
    wl_surface_commit(surface);

    while (wl_display_dispatch(display)) {
        if (closed) break;
    }

    if (egl_disp) {
        eglMakeCurrent(egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surf) eglDestroySurface(egl_disp, egl_surf);
        if (egl_window) wl_egl_window_destroy(egl_window);
        if (egl_ctx) eglDestroyContext(egl_disp, egl_ctx);
        eglTerminate(egl_disp);
    }

    xdg_toplevel_destroy(top);
    xdg_surface_destroy(x_surface);
    wl_surface_destroy(surface);
    wl_display_disconnect(display);
    return 0;
}
