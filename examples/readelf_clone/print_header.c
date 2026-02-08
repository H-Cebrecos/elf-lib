#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "common/elf_core.h"
#include "reader/elf_reader.h"
#include "enum2str.h"

static ElfResult file_read_cb(void *user_ctx,
                              uint64_t offset,
                              size_t size,
                              void *buffer)
{
    FILE *f = (FILE *)user_ctx;

    /* Seek to requested offset */
    if (fseeko(f, offset, SEEK_SET) != 0)
        return ELF_IO_ERROR;

    /* Read exactly `size` bytes */
    if (fread(buffer, 1, size, f) != size)
        return feof(f) ? ELF_IO_EOF : ELF_IO_ERROR;

    return ELF_OK;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    ElfCtx ctx; /* caller-allocated, opaque */

    ElfResult err = elf_init(f, file_read_cb, &ctx);
    if (err != ELF_OK)
    {
        fprintf(stderr, "elf_init failed: %s\n", elferr_to_str(err));
        fclose(f);
        return 1;
    }

    ElfHeader hdr;
    err = get_header(&ctx, &hdr);
    if (err != ELF_OK)
    {
        fprintf(stderr, "get_header failed: %s\n", elferr_to_str(err));
        fclose(f);
        return 1;
    }

    /* Print readelf-style summary */
    printf("ELF Header:\n");
    printf("  Class:                             %s\n", class_to_str(hdr.EI_Class));
    printf("  Data:                              %s\n", data_to_str(hdr.EI_Data));
    printf("  Version:                           %d (current)\n", EV_CURRENT);
    printf("  Version:                           %s\n", abi_to_str(hdr.EI_OS_ABI));
    printf("  ABI Version:                       %u\n", hdr.EI_ABI_Version);
    printf("  Type:                              %s\n", type_to_str(hdr.Type));
    printf("  Machine:                           %s\n", machine_to_str(hdr.Machine));
    printf("  Version:                           0x%" PRIx32 "\n", hdr.Version);
    printf("  Entry point address:               0x%" PRIx64 "\n", hdr.Entry);
    printf("  Start of program headers:          %" PRIu64 " (bytes into file)\n", hdr.ProHeadOff);
    printf("  Start of section headers:          %" PRIu64 " (bytes into file)\n", hdr.SecHeadOff);
    printf("  Flags:                             0x%" PRIx32 "\n", hdr.Flags);
    printf("  Size of this header:               %u (bytes)\n", hdr.HeadSize);
    printf("  Size of program headers:           %u (bytes)\n", hdr.PHEntrySize);
    printf("  Number of program headers:         %u\n", hdr.PHEntryNum);
    printf("  Size of section headers:           %u (bytes)\n", hdr.SHEntrySize);
    printf("  Number of section headers:         %u\n", hdr.SHEntryNum);
    printf("  Section header string table index: %u\n", hdr.SecStrIndx);

    printf("\nSection Headers:\n");
    printf("[Nr] Name                 Type       Addr             Off      Size     ES  Flg Lk Inf Al\n");

    for (uint16_t i = 0; i < get_section_count(&ctx); i++)
    {
        ElfSecHeader sh;
        if (get_section_header(&ctx, i, &sh) != ELF_OK)
            continue;

        uint8_t name[256];
        if (get_section_name(&ctx, &sh, name, sizeof(name)) != ELF_OK)
            strcpy((char *)name, "<error>");

        printf("[%2u] %-20s %-10u %016" PRIx64 " %08" PRIx64
               " %08" PRIx64 " %02" PRIx64 " %3" PRIx64 " %2u %3u %2" PRIx64 "\n",
               i,
               name,
               (uint32_t)sh.Type,
               sh.Address,
               sh.Offset,
               sh.Size,
               sh.EntrySize,
               sh.Flags,
               sh.Link,
               sh.Info,
               sh.Alignment);
    }

    printf("\nProgram Headers:\n");
    printf(" Type           Offset     VirtAddr   PhysAddr   FileSiz  MemSiz  Flags  Align\n");
    for (uint32_t i = 0; i < get_program_header_count(&ctx); i++)
    {
        ElfProHeader ph;
        if (get_program_header(&ctx, i, &ph) != ELF_OK)
            continue;

        printf(" %-14s %010" PRIx64 " %010" PRIx64 " %010" PRIx64
               " %08" PRIx64 " %07" PRIx64 " %5" PRIx32 " %6" PRIx64 "\n",
               segment_type_to_str(ph.Type),
               ph.Offset,
               ph.VirAddress,
               ph.PhyAddress,
               ph.FileSize,
               ph.MemSize,
               ph.Flags,
               ph.Alignment);
    }

    printf("\nSymbol Tables:\n");

    printf("\nSymbol Tables:\n");

    for (uint16_t i = 0; i < get_section_count(&ctx); i++)
    {
        ElfSecHeader sh;
        if (get_section_header(&ctx, i, &sh) != ELF_OK)
            continue;

        if (sh.Type != SHT_SYMTAB && sh.Type != SHT_DYNSYM)
            continue;

        uint8_t sec_name[256];
        get_section_name(&ctx, &sh, sec_name, sizeof(sec_name));

        printf("\nSymbol table '%s':\n", sec_name);
        printf(" Num:    Value          Size Type     Bind     Sec Name\n");

        uint64_t count = sh.Size / sh.EntrySize;
        for (uint64_t j = 0; j < count; j++)
        {
            ElfSymTabEntry sym;
            if (get_symbol_entry(&ctx, &sh, j, &sym) != ELF_OK)
                continue;

            uint8_t name[256];
            if (get_symbol_name(&ctx, sh.Link, &sym, name, sizeof(name)) != ELF_OK)
                strcpy((char *)name, "<err>");

            printf("%5" PRIu64 ": %016" PRIx64 " %5" PRIu64 " %-8s %-8s %3lu %s\n",
                   j,
                   sym.Value,
                   sym.Size,
                   sym_type_to_str(sym.Type),
                   sym_bind_to_str(sym.Binding),
                   sym.SecIdx,
                   name);
        }
    }

    fclose(f);
    return 0;
}