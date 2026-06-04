#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

void draw(void* data) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

i32 main(i32 argc, u8 **argv) {
    Graphics_Window* window = graphics_window_create();

    graphics_set_draw_callback(window, draw, 0L);
    while (graphics_poll_events(window));
    graphics_window_close(window);

    return 0;
}
