#include "misty.h"

MistyMountain* misty_mountain_create(Arena* arena) {
    MistyMountain* mountain = push_struct(arena, MistyMountain);
    mountain->arena = arena;
    return mountain;
}

#define HeaderGet(header, field) (((header).type == ELF_32) ? (header).h.h32.field : (header).h.h64.field)
MistyMountain_SectionHeaderTableInfo misty_read_elf_header(MistyMountain* mountain, File* f) {
    ElfHeader header;
    file_read_bytes(f, 0, &header.h, sizeof(header.h));

    u8* e_ident = HeaderGet(header, e_ident);
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
    mountain->shstrtab = 1 + HeaderGet(header, e_shstrndx);
    mountain->sections = Array(mountain->arena, 1 + HeaderGet(header, e_shnum), MistyMountain_Section);

    return (MistyMountain_SectionHeaderTableInfo) {
        .table_offset = HeaderGet(header, e_shoff),
        .entry_size = HeaderGet(header, e_shentsize),
    };
}
