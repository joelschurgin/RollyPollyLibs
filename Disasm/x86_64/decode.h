#ifndef DECODE_H
#define DECODE_H

#include "base.h"
#include "types.h"
#include "operands.h"
#include "flops.h"
#include "decode_two_byte_opcodes.h"

internal Disasm_Prefix _disasm_decode_prefix(u8** instr_ptr);
Disasm_Instr  disasm_decode(u8* instr_ptr);

#endif
