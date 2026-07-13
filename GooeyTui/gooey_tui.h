#ifndef GOOEY_TUI_H
#define GOOEY_TUI_H

#include "base.h"

#include <termios.h>
#include <sys/ioctl.h>

typedef struct {
    Arena* arena;
    struct termios old_terminal;
    i32 w;
    i32 h;
} GooeyTui;

void        gooey_tui_terminal_size_update(GooeyTui* gt);
GooeyTui*   gooey_tui_init(Arena* arena);
void        gooey_tui_release(GooeyTui** gt);

typedef struct {
    i64 x;
    i64 y;
    u64 num_repeats;
    b32 flush;
} GooeyTui_Write_Params;
void        gooey_tui_write_(GooeyTui* gt, String str, GooeyTui_Write_Params* params);
#define     gooey_tui_write(gt, str, ...) \
    gooey_tui_write_(gt, str, &(GooeyTui_Write_Params){ .x = -1, .y = -1, .flush = false, .num_repeats = 1, __VA_ARGS__ })

void        gooey_tui_clear_screen(GooeyTui* gt);
void        gooey_tui_move_cursor(GooeyTui* gt, i32 x, i32 y);

#define GooeyTuiBlock(arena, gooey_tui_var_name) \
    for (GooeyTui* gooey_tui_var_name = gooey_tui_init((arena)); gooey_tui_var_name; ) \
    DeferBlock({}, { gooey_tui_release(&gooey_tui_var_name); })

#endif
