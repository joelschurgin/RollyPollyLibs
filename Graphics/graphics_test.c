#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <wayland-egl.h>

#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include <GL/gl.h>
#include <EGL/egl.h>

#include <stdlib.h>

#define EGL_STATUS AssertAlways

struct wl_compositor* compositor = NULL;
struct wl_surface* surface = NULL;
struct wl_buffer* buf = NULL;
struct wl_shm* shm = NULL;
struct xdg_wm_base* shell = NULL;
struct xdg_toplevel* top = NULL;

u8* pixels = NULL;
u16 w = 200;
u16 h = 100;

i32 alloc_shm(u64 size) {
    i8 name[8];
    name[0] = '/';
    name[7] = 0;
    for (u8 i = 1; i <= 6; i++) {
        name[i] = (rand() % 23) + 97;
    }

    i32 fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
    shm_unlink(name);

    ftruncate(fd, size);

    return fd;
}

void resize() {
    u64 size = w * h * 4;

    i32 fd = alloc_shm(size);
    pixels = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
    buf = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);
}

void draw() {

}

void x_surface_conf(void* data, struct xdg_surface* x_surface, u32 serial) {
    xdg_surface_ack_configure(x_surface, serial);

    if (!pixels) {
        resize();
    }

    draw();
    wl_surface_attach(surface, buf, 0, 0);
    wl_surface_damage(surface, 0, 0, w, h);
    wl_surface_commit(surface);
}

struct xdg_surface_listener x_surface_listener = {
    .configure = x_surface_conf,
};

void reg_glob(void* data, struct wl_registry* reg, u32 name, const char* interface, u32 version) {
    if(!strcmp(interface, wl_compositor_interface.name)) {
        compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
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

    surface = wl_compositor_create_surface(compositor);

    struct xdg_surface* x_surface = xdg_wm_base_get_xdg_surface(shell, surface);
    xdg_surface_add_listener(x_surface, &x_surface_listener, 0);

    if (buf) {
        wl_buffer_destroy(buf);
    }

    wl_surface_destroy(surface);
    wl_display_disconnect(display);
    return 0;
}













    /*
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    AssertAlways(display != EGL_NO_DISPLAY);

    EGLint egl_version_major, egl_version_minor;

    EGL_STATUS(eglInitialize(display, &egl_version_major, &egl_version_minor));
    AssertAlways(egl_version_major == 1 && egl_version_minor <= 5);

    EGL_STATUS(eglBindAPI(EGL_OPENGL_API));

    get_window();

    b32 debug_mode = 0;
#ifdef BUILD_DEBUG
    debug_mode = 1;
#endif
    EGLint options[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        debug_mode ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE, EGL_TRUE,
        EGL_NONE,
    };
    EGLContext context = eglCreateContext(display, 0, EGL_NO_CONTEXT, options);
    EGL_STATUS(context != EGL_NO_CONTEXT);

    //EGL_STATUS(eglMakeCurrent(display, 0, 0, context));
    //glDrawBuffer(GL_BACK);

    //surface = eglCreateWindowSurface(display, , , NULL);
    //eglDestroyContext(display, context);
    eglTerminate(display);
    printf("Version %d.%d\n", egl_version_major, egl_version_minor);

    */
