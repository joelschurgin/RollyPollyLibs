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

typedef enum {
    DWARF_NONE = ELF_NONE,
    DWARF_32 = ELF_32,
    DWARF_64 = ELF_64,
} DwarfType;

typedef struct {
    String name;
    u64 size;
    u64 offset;
    u64 pos;
    DwarfType addr_type;
} Misty_Section;

DefineArray(Misty_Section);


// sbyte = i8
// ubyte = u8
// uhalf = u16
// uword = u32

typedef struct {
    DwarfLineContentType type;
    DwarfFormType form;
} Misty_LineHeaderEntryFormat;

DefineArray(Misty_LineHeaderEntryFormat);

typedef enum {
    MISTY_NONE,
    MISTY_STRING,
    MISTY_UINT,
    MISTY_ADDR,
} Misty_ValueType;

typedef struct {
    Misty_ValueType type;
    union {
        String str;
        u64 addr;
        u64 uint;
    } val;
} Misty_Value;

typedef struct {
    String file_name;
    u64 dir_idx;
} Misty_FilePath;

DefineArray(Misty_FilePath);

typedef struct {
    u64 unit_length;
    u16 version;
    u8 address_size_bytes;
    u8 segment_selector_size_bytes;

    u64 header_length;

    u8 min_instr_length;
    u8 max_ops_per_instr;
    u8 default_is_stmt;
    i8 line_base;
    u8 line_range;

    u8 opcode_base;
    u8Array standard_opcode_lengths;

    DwarfType type;
} Misty_LineInfoHeader;

typedef struct {
    u64 addr;
    u64 line;
    u64 column;
    u64 file_idx;
} Misty_LineInfo;

DefineArray(Misty_LineInfo);

typedef struct {
    Arena* arena;
    ElfType type;
    ElfEndian endian;

    u64 debug_str;
    u64 debug_str_offsets;
    u64 debug_line;
    u64 debug_line_str;

    Misty_SectionArray sections;

    StringArray dirs;
    Misty_FilePathArray file_paths;
} Misty;

Misty* misty_mountain_create(Arena* arena);
#define Misty(arena) misty_mountain_create((arena))

#define misty_section(misty_mountain, section_name) ((misty_mountain)->sections.data[(misty_mountain)->section_name])

typedef struct {
    u64 table_offset;
    u64 entry_size;
    u8* section_names;
} Misty_SectionHeaderTableInfo;

Misty_SectionHeaderTableInfo     misty_read_elf_header(Misty* mountain, File* f);
u8*                              misty_read_shstrtab(Misty* mountain, File* f, u64 offset);

void                             misty_read_elf_section_headers(Misty* mountain, File* f, Misty_SectionHeaderTableInfo section_header_table_info);

Misty_Value                      misty_read_form_val(Misty* mountain, File* f, Misty_Section* section, DwarfFormType form);
u64                              misty_read_initial_length_value(File* f, Misty_Section* section, DwarfType* type);

Misty_LineInfoHeader             misty_read_line_info_header(Misty* mountain, File* f, Misty_Section* section);
Misty_LineInfoArray              misty_read_line_info(Misty* mountain, File* f);

#endif
