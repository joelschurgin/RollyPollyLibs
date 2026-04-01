#include "misty.h"

MistyMountain* misty_mountain_create(Arena* arena) {
    MistyMountain* mountain = push_struct(arena, MistyMountain);
    mountain->arena = arena;
    return mountain;
}

#define ElfHeaderGet(header, field) (((header).type == ELF_32) ? (header).h.h32.field : (header).h.h64.field)
MistyMountain_SectionHeaderTableInfo misty_read_elf_header(MistyMountain* mountain, File* f) {
    ElfHeader header;
    file_read_bytes(f, 0, &header.h, sizeof(header.h));

    u8* e_ident = ElfHeaderGet(header, e_ident);
    if (e_ident[EI_MAG0] != 0x7f ||
        e_ident[EI_MAG1] != 'E'  ||
        e_ident[EI_MAG2] != 'L'  ||
        e_ident[EI_MAG3] != 'F') {
        Assert(!"Not a valid ELF!!!");
    }

    ElfType type = e_ident[EI_CLASS];
    header.type = type;
    Assert(type == ELFCLASS32 || type == ELFCLASS64);

    ElfEndian endian = e_ident[EI_DATA];
    Assert(endian != ELFDATANONE);

    mountain->type     = type;
    mountain->endian   = endian;
    mountain->sections = Array(mountain->arena, 1 + ElfHeaderGet(header, e_shnum), MistyMountain_Section);

    MistyMountain_SectionHeaderTableInfo section_header_table_info = (MistyMountain_SectionHeaderTableInfo) {
        .table_offset = ElfHeaderGet(header, e_shoff),
        .entry_size = ElfHeaderGet(header, e_shentsize),
    };

    u64 shstrtab_offset = section_header_table_info.table_offset + ElfHeaderGet(header, e_shstrndx) * section_header_table_info.entry_size;
    section_header_table_info.section_names = misty_read_shstrtab(mountain, f, shstrtab_offset);

    return section_header_table_info;
}

u8* misty_read_shstrtab(MistyMountain* mountain, File* f, u64 offset) {
    ElfSectionHeader section_header = (ElfSectionHeader){
        .type = mountain->type,
    };
    file_read_bytes(f, offset, &section_header.h, sizeof(section_header.h));

    u64 section_size = ElfHeaderGet(section_header, sh_size);
    u64 section_offset = ElfHeaderGet(section_header, sh_offset);

    u8* names = push_array(mountain->arena, u8, section_size, false);
    file_read_bytes(f, section_offset, names, section_size);
    return names;
}

void misty_read_elf_section_headers(MistyMountain* mountain, File* f, MistyMountain_SectionHeaderTableInfo section_header_table_info) {
    ThreadArraySplit split = ThreadArraySplit(mountain->sections.count-1);

    for (u64 section_idx = split.start_idx; section_idx < split.end_idx; section_idx++) {
        ElfSectionHeader section_header = (ElfSectionHeader){
            .type = mountain->type,
        };
        u64 section_header_offset = section_header_table_info.table_offset + section_idx * section_header_table_info.entry_size;
        file_read_bytes(f, section_header_offset, &section_header.h, sizeof(section_header.h));
 
        u64 section_name_idx = ElfHeaderGet(section_header, sh_name);

        MistyMountain_Section* section = &mountain->sections.data[section_idx+1];
        section->name = String(&section_header_table_info.section_names[section_name_idx]);
        printf("section_idx %d => name %s\n", section_idx, section->name.str);
    }
}
