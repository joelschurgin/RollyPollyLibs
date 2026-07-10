#include "gooey_tui.h"

void gooey_tui_terminal_size_update(GooeyTui* gt) {
    struct winsize win;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0) {
        gt->w = win.ws_col;
        gt->h = win.ws_row;
    }
}

GooeyTui* gooey_tui_init(Arena* arena) {
    if (!arena) arena = arena_alloc(KB(16), KB(4));

    GooeyTui* gt = push_struct(arena, GooeyTui);

    tcgetattr(STDIN_FILENO, &gt->old_terminal);
 
    struct termios term;
    term.c_iflag &= ~(BRKINT | ICRNL | INLCR | ISTRIP | IXON);
    term.c_oflag &= ~(OPOST);
    term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);

    gooey_tui_terminal_size_update(gt);

    gt->arena = arena;

    return gt;
}

void gooey_tui_release(GooeyTui** gt) {
    Assert(gt);
    Assert(*gt);

    //printf("\033[2J\033[H");
    //printf("\033]112\007");
    //printf("\033[0m");
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &((*gt)->old_terminal));

    *gt = 0L;
}

internal void _gooey_tui_output_string(String str) {
    AssertAlways(write(STDOUT_FILENO, str.str, str.size) == (i64)str.size);
}

void gooey_tui_output_string_(GooeyTui* gt, String str, GooeyTui_OutputString_Params* params) {
    gooey_tui_move_cursor(gt, params->x, params->y);
    _gooey_tui_output_string(str);
    if (params->flush) fflush(stdout);
}

void gooey_tui_clear_screen(GooeyTui* gt) {
    gooey_tui_output_string(gt, String("\033[2J\033[H"), .flush = true);
}

void gooey_tui_move_cursor(GooeyTui* gt, i32 x, i32 y) {
    if (x < 0 || y < 0) return;

    TempArenaBlock(gt->arena) {
        String cursor_pos = string_format(gt->arena, "\033[%d;%dH", y+1, x+1);
        gooey_tui_output_string(gt, cursor_pos, .flush = false);
    }
}
