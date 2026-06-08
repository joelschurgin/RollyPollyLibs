#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

void draw(void* data) {
    Graphics_Window* window = (Graphics_Window*)data;

    u32 width = 0;
    u32 height = 0;
    graphics_window_dimensions(window, &width, &height);

    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    graphics_rect_fill(window, &(Graphics_Rect) {
        .x = width / 4,
        .y = height / 4,
        .w = width / 2,
        .h = height / 2,
        .radius = 60,
        .border_thickness = 1.0f,
        .fill_color = Graphics_Color(0.0f, 0.5f, 0.5f, 1.0f),
        .border_color = Graphics_Color(1.0f, 1.0f, 1.0f, 1.0f),
    });
}

i32 main(i32 argc, u8 **argv) {
    Graphics_Window* window = graphics_window_create();

    graphics_set_draw_callback(window, draw, window);
    while (graphics_poll_events(window));
    graphics_window_close(window);

    return 0;
}
