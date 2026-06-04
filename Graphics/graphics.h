#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <wayland-egl.h>
#include <GL/gl.h>
#include <EGL/egl.h>

#include "base.h"

extern Arena* graphics_arena;

typedef struct Graphics_Window Graphics_Window;
typedef void (*graphics_draw_func_t)(void* data);

void                    graphics_init();
Graphics_Window*        graphics_window_create();
void                    graphics_window_close(Graphics_Window* window);
b32                     graphics_poll_events(Graphics_Window* window);

void                    graphics_set_draw_callback(Graphics_Window* window, graphics_draw_func_t draw_func, void* data);
