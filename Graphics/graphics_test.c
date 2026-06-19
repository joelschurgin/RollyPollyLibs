#include "graphics.h"

#define BASE_ENTRY_POINT
#include "base.h"

typedef struct {
    Graphics_Window* window;
    Graphics_Font* font;
} Data;

void draw(void* data) {
    Graphics_Window* window = ((Data*)data)->window;
    Graphics_Font* font = ((Data*)data)->font;

    u32 width = 0;
    u32 height = 0;
    graphics_window_dimensions(window, &width, &height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /*
    graphics_rect_fill(window, (f32)width / 4.0f,
                               (f32)height / 4.0f,
                               (f32)width / 2.0f,
                               (f32)height / 2.0f,
                               60.0f, 1.0f,
                               Graphics_Color(0.0f, 0.5f, 0.5f, 1.0f),
                               Graphics_Color(1.0f, 1.0f, 1.0f, 1.0f));
    */

    /*
    graphics_image_rect_draw(window, 0.0f,
                                     0.0f,
                                     (f32)width,
                                     (f32)height);
    */

    graphics_font_draw(window, font, String("Hello World!"), 0.0f, (f32)height / 2.0f, 60.0f);
}

i32 main(i32 argc, u8 **argv) {
    Graphics_Window* window = graphics_window_create();
    Graphics_Font* font = graphics_font_load(String("Graphics/Fonts/RobotoMono"));

    graphics_set_draw_callback(window, draw, &(Data){ .window = window, .font = font });
    while (graphics_poll_events(window));
    graphics_window_close(window);

    return 0;
}
