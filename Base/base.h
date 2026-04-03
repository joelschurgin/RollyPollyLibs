#ifndef BASE_H
#define BASE_H

#include "base_context_cracking.h"

C_LINKAGE_BEGIN

#include "base_core.h"
#include "base_arena.h"
#include "base_thread.h"
#include "base_string.h"
#include "base_file.h"

C_LINKAGE_END

#endif

#ifdef BASE_ENTRY_POINT
#include "base_entry_point.h"
#endif
