/*
 * Copyright (c) 2025 Hugo Cebrecos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "common/elf_core.h"
#include "reader/elf_reader.h" 

static ElfResult file_read_cb(void *user_ctx, uint64_t offset, size_t size, void *buffer)
{
    FILE *f = (FILE *)user_ctx;
    if (fseeko(f, offset, SEEK_SET) != 0)
    {
        return ELF_IO_ERROR;
    }
    size_t r = fread((void *)buffer, 1, size, f);
    if (r < size)
    {
        if (feof(f))
            return ELF_IO_EOF;
        return ELF_IO_ERROR;
    }
    return ELF_OK;
}

static const char *class_to_str(EiClass c)
{
    switch (c)
    {
    case ELFCLASS32:
        return "ELF32";
    case ELFCLASS64:
        return "ELF64";
    default:
        return "none";
    }
}

static const char *data_to_str(EiData d)
{
    switch (d)
    {
    case ELFDATA2LSB:
        return "2's complement, little endian";
    case ELFDATA2MSB:
        return "2's complement, big endian";
    default:
        return "unknown";
    }
}

static const char *type_to_str(ElfType t)
{
    switch (t)
    {
    case ET_REL:
        return "REL (Relocatable file)";
    case ET_EXEC:
        return "EXEC (Executable file)";
    case ET_DYN:
        return "DYN (Shared object file)";
    case ET_CORE:
        return "CORE (Core file)";
    default:
        return "NONE";
    }
}

static const char *machine_to_str(ElfMachine m)
{
    switch (m)
    {
    //case :
    //    return "ARM";
    //case MACHINE_RISCV:
    //    return "RISC-V";
    //case MACHINE_AARCH64:
    //    return "AArch64";
    default:
        return "Unknown";
    }
}

static const char *abi_to_str(ElfABI m)
{
    switch (m)
    {
    case ELFOSABI_NONE:
        return "UNIX - SYSTEM V";
    default:
        return "Unknown";
    }
}

static const char *elferr_to_str(ElfResult e)
{
    switch (e)
    {
    case ELF_OK:
        return "OK";
    case ELF_BAD_MAGIC:
        return "Bad magic";
    case ELF_BAD_VERSION:
        return "Bad version";
    case ELF_BAD_CLASS:
        return "Bad class";
    case ELF_BAD_ENDIANNESS:
        return "Bad endianness";
    case ELF_IO_ERROR:
        return "I/O error";
    default:
        return "Unknown error";
    }
}

static const char *segment_type_to_str(ElfSegmentType t)
{
    switch (t)
    {
    case SEGMENT_NULL:
        return "NULL";
    case SEGMENT_LOAD:
        return "LOAD";
    case SEGMENT_DYNAMIC:
        return "DYNAMIC";
    case SEGMENT_INTERP:
        return "INTERP";
    case SEGMENT_NOTE:
        return "NOTE";
    case SEGMENT_SHLIB:
        return "SHLIB";
    case SEGMENT_PHDR:
        return "PHDR";
    default:
        return "UNKNOWN";
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 2;
    }

    const char *fname = argv[1];
    FILE *f = fopen(fname, "rb");
    if (!f)
    {
        perror("fopen");
        return 2;
    }

    ElfCtx ctx; /* caller-allocated opaque context */
    ElfResult err = elf_init((void *)f, file_read_cb, &ctx);
    if (err != ELF_OK)
    {
        fprintf(stderr, "elf_init failed: %s\n", elferr_to_str(err));
        fclose(f);
        return 3;
    }

    ElfHeader hdr;
    err = get_header(&ctx, &hdr);
    if (err != ELF_OK)
    {
        fprintf(stderr, "parse_header failed: %s\n", elferr_to_str(err));
        fclose(f);
        return 4;
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

    for (uint16_t i = 0; i < hdr.SHEntryNum; i++)
    {
        ElfSecHeader sh;
        ElfResult err = get_section_header(&ctx, i, &sh);
        if (err != ELF_OK)
        {
            fprintf(stderr, "[%2u] <error reading section header: %s>\n",
                    i, elferr_to_str(err));
            continue;
        }

        uint8_t name[256];
        ElfResult nerr = get_section_name(&ctx, &sh, name, sizeof(name));
        if (nerr != ELF_OK)
        {
            snprintf((char *)name, sizeof(name), "<error: %s>", elferr_to_str(nerr));
        }

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
    for (uint32_t i = 0; i < hdr.PHEntryNum; i++)
    {
        ElfProHeader ph;
        ElfResult perr = get_program_header(&ctx, i, &ph);
        if (perr != ELF_OK)
        {
            fprintf(stderr, "[%2u] <error: %s>\n", i, elferr_to_str(perr));
            continue;
        }

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

    for (uint32_t i = 0; i < hdr.SHEntryNum; i++)
    {
        ElfSecHeader sh;
        if (get_section_header(&ctx, i, &sh) != ELF_OK)
            continue;

        if (sh.Type != SHT_SYMTAB && sh.Type != SHT_DYNSYM)
            continue;

        uint8_t sec_name[256];
        get_section_name(&ctx, &sh, sec_name, sizeof(sec_name));
        printf("\nSymbol table '%s' contains %" PRIu64 " entries:\n", sec_name,
               (uint64_t)(get_symbol_count(&ctx, &sh)));
        printf("   Num:    Value          Size Type     Bind     SectIdx Name\n");

        for (uint64_t j = 0; j < sh.Size / sh.EntrySize; j++)
        {
            ElfSymTabEntry sym;
            if (get_symbol_entry(&ctx, &sh, j, &sym) != ELF_OK)
                continue;

            uint8_t symname[256];
            ElfResult sres = get_symbol_name(&ctx, sh.Link, &sym, symname, sizeof(symname));
            if (sres != ELF_OK)
                snprintf((char *)symname, sizeof(symname), "<err>");

            const char *type_str;
            switch (sym.Type)
            {
            case STT_NOTYPE:
                type_str = "NOTYPE";
                break;
            case STT_OBJECT:
                type_str = "OBJECT";
                break;
            case STT_FUNC:
                type_str = "FUNC";
                break;
            case STT_SECTION:
                type_str = "SECTION";
                break;
            case STT_FILE:
                type_str = "FILE";
                break;
            default:
                type_str = "UNK";
                break;
            }

            const char *bind_str;
            switch (sym.Binding)
            {
            case STB_LOCAL:
                bind_str = "LOCAL";
                break;
            case STB_GLOBAL:
                bind_str = "GLOBAL";
                break;
            case STB_WEAK:
                bind_str = "WEAK";
                break;
            default:
                bind_str = "UNK";
                break;
            }

            printf("%6" PRIu64 ": %016" PRIx64 " %5" PRIu64 " %-8s %-9s %6u %s\n",
                   j, sym.Value, sym.Size, type_str, bind_str, sym.SecIdx, symname);
        }
    }

    fclose(f);
    return 0;
}
