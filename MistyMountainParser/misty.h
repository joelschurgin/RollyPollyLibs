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
    u64Array pos;
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
    MISTY_LOCATION_NONE,
    MISTY_LOCATION_FBREG,
} Misty_LocationType;

typedef struct {
    union {
        i64 sint;
    } val;
    Misty_LocationType type;
} Misty_Location;

typedef enum {
    MISTY_NONE,
    MISTY_STRING,
    MISTY_UINT,
    MISTY_SINT,
    MISTY_ADDR,
    MISTY_REF,
    MISTY_FLAG,
    MISTY_LOCATION,
} Misty_ValueType;

typedef struct {
    Misty_ValueType type;
    union {
        String str;
        u64 addr;
        u64 uint;
        i64 sint;
        u64 offset;
        b8 flag;
        Misty_Location location;
    } val;
} Misty_Value;

typedef enum {
    MISTY_TYPE_INFO_MODIFIER_NONE,
    MISTY_CONST,
    MISTY_POINTER,
} Misty_TypeInfoModifier;

typedef struct Misty_TypeInfo Misty_TypeInfo;
struct Misty_TypeInfo {
    Misty_TypeInfo* next;
    Misty_TypeInfo* related_type_info;
    String name;
    u64 byte_size;
    DwarfAttributeEncodingType encoding;
    Misty_TypeInfoModifier modifier;
    u64 entry_addr;
    u64 type_ref;
};

typedef struct {
    Misty_TypeInfo* first;
    Misty_TypeInfo* last;
    u64 size;
} Misty_TypeInfoList;

typedef struct Misty_Variable Misty_Variable;
struct Misty_Variable {
    Misty_Variable* next;

    Misty_TypeInfo* type;
    String name;

    u64 file_idx;
    u64 line;
    u64 column;
 
    u64 entry_addr;

    u64 type_ref;

    Misty_Location location;
};

typedef struct {
    Misty_Variable* first;
    Misty_Variable* last;
    u64 size;
} Misty_VariableList;

DefineArray(Misty_Variable);

typedef struct Misty_VariableRef Misty_VariableRef;
struct Misty_VariableRef {
    Misty_VariableRef* next;
    Misty_Variable* ref;
};

typedef struct {
    Misty_VariableRef* first;
    Misty_VariableRef* last;
} Misty_VariableRefList;

typedef struct Misty_Subprogram Misty_Subprogram;
struct Misty_Subprogram {
    Misty_Subprogram* next;

    String name;

    u64 file_idx;
    u64 line;
    u64 column;

    b8 prototyped;
    b8 external;

    u64 entry_addr;

    u64 ret_type_ref;

    Misty_VariableRefList parameters;

    b8 unspecified_params;
};

typedef struct {
    Misty_Subprogram* first;
    Misty_Subprogram* last;
    u64 size;
} Misty_SubprogramList;

typedef struct Misty_LexicalBlock Misty_LexicalBlock;
struct Misty_LexicalBlock {
    Misty_LexicalBlock* next;
    u64 low_pc;
    u64 high_pc;
    u64 entry_addr;
};

typedef struct {
    Misty_LexicalBlock* first;
    Misty_LexicalBlock* last;
    u64 size;
} Misty_LexicalBlockList;

typedef struct {
    String name;
    String producer;
    String language_name;
    String comp_dir;

    u64 language_version;

    u64 low_pc;
    u64 high_pc;

    u64 stmt_list;
    u64 macros;

    DwarfLang language;
} Misty_CompileUnit;

typedef struct {
    String file_name;
    u64 dir_idx;
} Misty_FilePath;

DefineArray(Misty_FilePath);

typedef struct {
    u64 unit_length;
    u16 version;
    u8 addr_size_bytes;
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
    u64 unit_length;
    u16 version;
    DwarfUnitType unit_type;
    u8  addr_size_bytes;
    u64 debug_abbrev_offset;

    u64 header_length;

    DwarfType type;
} Misty_DieHeader;

typedef enum {
    MISTY_DIE_ENTRY_NONE,
    MISTY_DIE_ENTRY_COMPUNIT,
    MISTY_DIE_ENTRY_TYPE,
    MISTY_DIE_ENTRY_SUBPROGRAM,
    MISTY_DIE_ENTRY_VARIABLE,
    MISTY_DIE_ENTRY_LEXICAL_BLOCK,
} Misty_DieEntryType;

typedef struct {
    void* value;
    Misty_DieEntryType type;
    DwarfHasChild has_child;
} Misty_DieEntry;

typedef struct {
    Misty_DieHeader header;
    u64Array abbrev_offsets;

    Misty_CompileUnit comp_unit;
 
    Misty_TypeInfoList type_info;
    Misty_VariableList variable;
    Misty_SubprogramList subprogram;
    Misty_LexicalBlockList lexical_block;

    u64 start_addr;
} Misty_Die;

DefineArray(Misty_Die);

typedef struct {
    Arena* temp_arena;
    Arena* dest_arena;
    ElfType type;
    ElfEndian endian;

    u64 debug_str;
    u64 debug_str_offsets;
    u64 debug_line;
    u64 debug_line_str;
    u64 debug_abbrev;
    u64 debug_info;

    Misty_SectionArray sections;

    StringArray dirs;
    Misty_FilePathArray file_paths;

    Mutex* mutex;
} Misty_DwarfReader;

Misty_DwarfReader* misty_mountain_create(Arena* dest_arena, Arena* temp_arena);

#define misty_section(misty_mountain, section_name) ((misty_mountain)->sections.data[(misty_mountain)->section_name])

typedef struct {
    u64 table_offset;
    u64 entry_size;
    u8* section_names;
} Misty_SectionHeaderTableInfo;

typedef struct {
    Arena* arena;

    StringArray dirs;
    Misty_FilePathArray file_paths;

    Misty_LineInfoArray line_info;
} Misty_DebugInfo;

Misty_SectionHeaderTableInfo     misty_read_elf_header(Misty_DwarfReader* mountain, File* f);
u8*                              misty_read_shstrtab(Misty_DwarfReader* mountain, File* f, u64 offset);

void                             misty_read_elf_section_headers(Misty_DwarfReader* mountain, File* f, Misty_SectionHeaderTableInfo section_header_table_info);

Misty_Value                      misty_read_form_val(Misty_DwarfReader* mountain, File* f, Misty_Section* section, DwarfFormType form, u8 addr_size_bytes);
u64                              misty_read_initial_length_value(File* f, Misty_Section* section, DwarfType* type);

Misty_LineInfoHeader             misty_read_line_info_header(Misty_DwarfReader* mountain, File* f, Misty_Section* section);
Misty_LineInfoArray              misty_read_line_info(Misty_DwarfReader* mountain, File* f);

void                             misty_die_read_abbrev_offsets(Misty_DwarfReader* mountain, File* f, Misty_Die* die);

void                             misty_parse_die(Misty_DwarfReader* mountain, File* f, Misty_Die* die, Misty_DieEntry* parent);
void                             misty_read_debug_info(Misty_DwarfReader* mountain, File* f);

Misty_DebugInfo*                 misty_parse(Arena* arena, String path);

#endif
