#include "graphics.h"

#include "xdg-shell-protocol.c"

global Arena* graphics_arena = NULL;

void graphics_init() {
    graphics_arena = default_arena();
}
