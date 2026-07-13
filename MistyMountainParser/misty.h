#ifndef MISTY_H
#define MISTY_H

#include "base.h"
#include "dwarf.h"
#include "elf.h"

typedef enum {
    ELF_NONE = ELFCLASSNONE,
    ELF_32 = ELFCLASS32,
    ELF_64 = ELFCLASS64,
} ElfType;

typedef enum {
    ELF_LSB = ELFDATA2LSB,
    ELF_MSB = ELFDATA2MSB,
} ElfEndian;

typedef struct {
    ElfType type;
    union {
        Elf32_Ehdr h32;
        Elf64_Ehdr h64;
    } h;
} ElfHeader;

typedef struct {
    ElfType type;
    union {
        Elf32_Shdr h32;
        Elf64_Shdr h64;
    } h;
} ElfSectionHeader;

typedef struct {
    String name;
    u64 size;
    u64 offset;
    u64 pos;
} MistyMountain_Section;

DefineArray(MistyMountain_Section);

typedef struct {
    Arena* arena;
    ElfType type;
    ElfEndian endian;

    u64 debug_str;
    u64 debug_str_offsets;
    u64 debug_line;

    MistyMountain_SectionArray sections;
} MistyMountain;

MistyMountain* misty_mountain_create(Arena* arena);
#define MistyMountain(arena) misty_mountain_create((arena))

#define misty_section(misty_mountain, section_name) &((misty_mountain)->sections.data[(misty_mountain)->section_name])

typedef struct {
    u64 table_offset;
    u64 entry_size;
    u8* section_names;
} MistyMountain_SectionHeaderTableInfo;
MistyMountain_SectionHeaderTableInfo misty_read_elf_header(MistyMountain* mountain, File* f);
u8* misty_read_shstrtab(MistyMountain* mountain, File* f, u64 offset);

void misty_read_elf_section_headers(MistyMountain* mountain, File* f, MistyMountain_SectionHeaderTableInfo section_header_table_info);

// sbyte = i8
// ubyte = u8
// uhalf = u16
// uword = u32

#endif
