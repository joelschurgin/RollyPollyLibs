#ifndef MOONFRUIT_H
#define MOONFRUIT_H

#include "base.h"

/*

#define ABC something
#define AAC something_else



 */

typedef struct {
    File* file;
    u64 size;
    u64 pos;
    u8* data;
} MoonFruit_Chunk;

#endif
