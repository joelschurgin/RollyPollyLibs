internal void _graphics_resize(Graphics_Window* window);
internal void _graphics_draw(Graphics_Window* window);

internal void _graphics_top_configure(void* data, struct xdg_toplevel* top, i32 new_width, i32 new_height, struct wl_array* stat);
internal void _graphics_top_close(void* data, struct xdg_toplevel* top);

internal void _graphics_x_surface_configure(void* data, struct xdg_surface* x_surface, u32 serial);

internal void _graphics_frame_callback(void* data, struct wl_callback* callback, u32 callback_data);

internal void _graphics_regstry_global(void* data, struct wl_registry* reg, u32 name, const char* interface, u32 version);
internal void _graphics_regstry_global_remove(void* data, struct wl_registry* reg, u32 name);

internal void _graphics_shell_ping(void* data, struct xdg_wm_base* sh, u32 serial);

internal struct xdg_toplevel_listener _graphics_top_listener = {
    .configure = _graphics_top_configure,
    .close = _graphics_top_close,
};

internal struct xdg_surface_listener _graphics_x_surface_listener = {
    .configure = _graphics_x_surface_configure,
};

internal const struct wl_callback_listener _graphics_frame_listener = {
    .done = _graphics_frame_callback,
};

struct xdg_wm_base_listener sh_listener = {
    .ping = _graphics_shell_ping,
};

#undef global
static struct wl_registry_listener _graphics_registry_listener = {
    .global = _graphics_regstry_global,
    .global_remove = _graphics_regstry_global_remove,
};
#define global static

internal void _graphics_resize(Graphics_Window* window) {
    if (!window->egl_window) {
        window->egl_window = wl_egl_window_create(window->surface, window->width, window->height);
        window->egl_surf = eglCreateWindowSurface(window->egl_disp, window->egl_cfg, window->egl_window, NULL);
        eglMakeCurrent(window->egl_disp, window->egl_surf, window->egl_surf, window->egl_ctx);
    } else {
        wl_egl_window_resize(window->egl_window, window->width, window->height, 0, 0);
    }
}

internal void _graphics_draw(Graphics_Window* window) {
    if (window->draw_func) window->draw_func(window->draw_func_data);


    struct wl_callback* callback = wl_surface_frame(window->surface);
    wl_callback_add_listener(callback, &_graphics_frame_listener, window);

    eglSwapBuffers(window->egl_disp, window->egl_surf);
}

internal void _graphics_top_configure(void* data, struct xdg_toplevel* top, i32 new_width, i32 new_height, struct wl_array* stat) {
    if (!new_width && !new_height) {
        return;
    }

    Graphics_Window* window = (Graphics_Window*)data;

    if (window->width != (u32)new_width || window->height != (u32)new_height) {
        window->width = new_width;
        window->height = new_height;
        _graphics_resize(window);
    }
}

internal void _graphics_top_close(void* data, struct xdg_toplevel* top) {
    Graphics_Window* window = (Graphics_Window*)data;
    window->closed = 1;
}

internal void _graphics_x_surface_configure(void* data, struct xdg_surface* x_surface, u32 serial) {
    xdg_surface_ack_configure(x_surface, serial);

    Graphics_Window* window = (Graphics_Window*)data;
    if (!window->egl_surf) _graphics_resize(window);
    _graphics_draw(window);
}

internal void _graphics_frame_callback(void* data, struct wl_callback* callback, u32 callback_data) {
    wl_callback_destroy(callback);

    Graphics_Window* window = (Graphics_Window*)data;
    if (window->closed) return;
    _graphics_draw(window);
}

internal void _graphics_shell_ping(void* data, struct xdg_wm_base* sh, u32 serial) {
    xdg_wm_base_pong(sh, serial);
}

internal void _graphics_regstry_global(void* data, struct wl_registry* reg, u32 name, const char* interface, u32 version) {
    Graphics_Window* window = (Graphics_Window*)data;
    if(!strcmp(interface, wl_compositor_interface.name)) {
        window->compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
    } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        window->shell = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(window->shell, &sh_listener, 0);
    }
}

internal void _graphics_regstry_global_remove(void* data, struct wl_registry* reg, u32 name) {}

internal VoidProc* _graphics_load_opengl_func(char *name) {
    VoidProc *result = (VoidProc *)eglGetProcAddress(name);
    return result;
}


