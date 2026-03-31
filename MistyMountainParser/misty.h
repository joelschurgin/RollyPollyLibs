#ifndef MISTY_H
#define MISTY_H

#include "base_headers.h"
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
    String name;
    u64 offset;
    u64 pos;
    void* data;
} MistyMountain_Section;

typedef MistyMountain_Section* MistyMountain_SectionArray;

typedef struct {
    Arena* arena;
    ElfType type;
    ElfEndian endian;

    u64 debug_abbrev_idx;
    u64 shstrtab;

    MistyMountain_SectionArray sections;
} MistyMountain;

MistyMountain* misty_mountain_create(Arena* arena);
#define MistyMountain(arena) misty_mountain_create((arena))

#define misty_section(misty_mountain, section_name) (misty_mountain)->sections[(misty_mountain)->section_name]

typedef struct {
    u64 table_offset;
    u64 entry_size;
} MistyMountain_SectionHeaderTableInfo;
MistyMountain_SectionHeaderTableInfo misty_read_elf_header(MistyMountain* mountain, File* f);

#endif
