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

//? NOTE: This is an internal file to host common structures to various elf related files, it should not be included into your project.

#ifndef ELF_REPR
#define ELF_REPR

#include <stdint.h>
#include "elf_core.h"

/**
 * This file contains the low level file representations of the structures as they appear in the ELF specification,
 * some of the "standard" enums are placed in elf_core, that is because they are enums intended for the consumer/producer of 
 * the ELF files, values that are mostly used internally are define in this file.
 */

#define EI_NIDENT 16


/****************
 *  ELF Header  *
 ****************/
        typedef struct
        {
                uint8_t Magic[4];       // 0x7f, E, L, F
                uint8_t EI_Class;       // 32 or 64 bit architecture
                uint8_t EI_Data;        // LSB or MSB architecture
                uint8_t EI_Version;     // Always 1
                uint8_t EI_OS_ABI;      // Target Platform's ABI
                uint8_t EI_ABI_Version; // Target ABI Version
                uint8_t Pad[7];
        } ElfInfo;

        _Static_assert(sizeof(ElfInfo) == EI_NIDENT, "e_ident size mismatch");

        typedef struct {
                union {
                        uint8_t  e_ident[EI_NIDENT];    // Standard definition
                        ElfInfo  info;                  // Easier to use alias
                };
                uint16_t e_type;        // Type of ELF file
                uint16_t e_machine;     // Architecture
                uint32_t e_version;     // Always 1
                uint32_t e_entry;       // Entry point (virtual address)
                uint32_t e_phoff;       // Offset of program header table in the file (bytes)
                uint32_t e_shoff;       // Offset of section header table in the file (bytes)
                uint32_t e_flags;       
                uint16_t e_ehsize;      // This header's size
                uint16_t e_phentsize;   // Size of one entry in the program header table
                uint16_t e_phnum;       // Number of entries in the program header table
                uint16_t e_shentsize;   // Size of one entry in the section header table
                uint16_t e_shnum;       // Number of entries in the section header table
                uint16_t e_shstrndx;    // Index of the entry in the section table that points to the section names.
        } Elf32Header;

        typedef struct
        {
                union {
                        uint8_t  e_ident[EI_NIDENT];    // Standard definition
                        ElfInfo  info;                  // Easier to use alias
                };
                uint16_t e_type;        // Type of ELF file
                uint16_t e_machine;     // Architecture
                uint32_t e_version;     // Always 1
                uint64_t e_entry;       // Entry point (virtual address)
                uint64_t e_phoff;       // Offset of program header table in the file (bytes)
                uint64_t e_shoff;       // Offset of section header table in the file (bytes)
                uint32_t e_flags;       
                uint16_t e_ehsize;      // This header's size
                uint16_t e_phentsize;   // Size of one entry in the program header table
                uint16_t e_phnum;       // Number of entries in the program header table
                uint16_t e_shentsize;   // Size of one entry in the section header table
                uint16_t e_shnum;       // Number of entries in the section header table
                uint16_t e_shstrndx;    // Index of the entry in the section table that points to the section names.
        } Elf64Header;

/****************
 *   Sections   *
 ****************/   
        typedef enum SecIndx 
        {
                SHN_UNDEF       = 0,
                SHN_LORESERVE   = 0xff00,       // if    e_shnum is >= than this value the field is zero       and the null sh contains the real value in sh_size
                                                // if e_shstrndx is >= than this value the field is SHN_XINDEX and the null sh contains the real value in sh_link

                /* Technically psABIs could define additional values here  */
                SHN_HIPROC      = 0xff1f,
                SHN_LOOS        = 0xff20,
                /* Technically OS ABIs could define additional values here  */
                SHN_HIOS        = 0xff3f,
                SHN_ABS         = 0xfff1,       // Symbols relative to this section are absolute and not relocatable
                SHN_COMMON      = 0xfff2,       // Symbols realitve to this section are common symbols (FORTRAN COMMON or unalloc extern C vars)
                SHN_XINDEX      = 0xffff,
        } SecIndx;

        typedef struct
        {
                uint32_t sh_name;       // Index into the section header string table section
                uint32_t sh_type;       // Type of section
                uint32_t sh_flags;
                uint32_t sh_addr;       // If the section is in the memory img of a process this is the first address
                uint32_t sh_offset;     // Section is stored at <offset> from the begining of this file
                uint32_t sh_size;       // Size of the section
                uint32_t sh_link;
                uint32_t sh_info;
                uint32_t sh_addralign;  // Alignment constraints of Address field
                uint32_t sh_entsize;    // If the section is a table of fixed-size entries this is the entry size
        } Elf32SecHeader;

        typedef struct
        {
                uint32_t sh_name;       // Index into the section header string table section
                uint32_t sh_type;       // Type of section
                uint64_t sh_flags;
                uint64_t sh_addr;       // If the section is in the memory img of a process this is the first address
                uint64_t sh_offset;     // Section is stored at <offset> from the begining of this file
                uint64_t sh_size;       // Size of the section
                uint32_t sh_link;
                uint32_t sh_info;
                uint64_t sh_addralign;  // Alignment constraints of Address field
                uint64_t sh_entsize;    // If the section is a table of fixed-size entries this is the entry size
        } Elf64SecHeader;

/**************** //TODO: give compression headers their abstract view in the core and support.
 *  Compression *
 ****************/   
        typedef enum CompressionType
        {
                ELFCOMPRESS_ZLIB   = 1,
                ELFCOMPRESS_ZSTD   = 2,
                ELFCOMPRESS_LOOS   = 0x60000000,
                /* Technically OS ABIs could define additional values here  */
                ELFCOMPRESS_HIOS   = 0x6fffffff,
                ELFCOMPRESS_LOPROC = 0x70000000,
                /* Technically psABIs could define additional values here  */
                ELFCOMPRESS_HIPROC = 0x7fffffff,
        }CompressionType;

        typedef struct
        {
                uint32_t ch_type;
                uint32_t ch_size;
                uint32_t ch_addralign;
        }Elf32CompressionHdr;

        typedef struct
        {
                uint32_t ch_type;
                uint32_t ch_reserved;
                uint64_t ch_size;
                uint64_t ch_addralign;
        }Elf64CompressionHdr;

/***************
 *   Symbols   *
 ***************/
        typedef enum SymbIndx {
                STN_UNDEF = 0,
        }SymbIndx;


        typedef struct {
                uint32_t st_name;       // Index into the related string table
                uint32_t st_value;      // Value (Address) of the symbol
                uint32_t st_size;       // Size of the object referenced by the symbol
                uint8_t  st_info;
                uint8_t  st_other;
                uint16_t st_shndx;
        }Elf32SymEntry;

        typedef struct {
                uint32_t st_name;       // Index into the related string table
                uint8_t  st_info;
                uint8_t  st_other;
                uint16_t st_shndx;
                uint64_t st_value;      // Value (Address) of the symbol
                uint16_t st_size;       // Size of the object referenced by the symbol
        }Elf64SymEntry;








typedef struct
{
        uint32_t Type; // Type of segment
        uint32_t Offset;     // Section is stored at <offset> from the begining of this file
        uint32_t PhyAddress; // Physical address, only relevant on some systems
        uint32_t VirAddress; // Virtual address of this segment in memory
        uint32_t FileSize;   // Size of the segment in this file
        uint32_t MemSize;    // Size of the segment in the memory image
        uint32_t Flags;      
        uint32_t Alignment;  // Alignment constraints of Address fields
} Elf32ProHeader;

typedef struct
{
        uint32_t Type; // Type of segment
        uint32_t Flags;      
        uint64_t Offset;     // Section is stored at <offset> from the begining of this file
        uint64_t PhyAddress; // Physical address, only relevant on some systems
        uint64_t VirAddress; // Virtual address of this segment in memory
        uint64_t FileSize;   // Size of the segment in this file
        uint64_t MemSize;    // Size of the segment in the memory image
        uint64_t Alignment;  // Alignment constraints of Address fields
} Elf64ProHeader;

typedef struct
{
        uint32_t NameIdx;      // Index into the related string table
        uint32_t Value;        // Value (Address) of the symbol
        uint32_t Size;         // Size of the object referenced by the symbol
        uint8_t  info;
        uint8_t  reserved;
        uint16_t SecIdx;
} Elf32SymEntry;

typedef struct
{
        uint32_t NameIdx;      // Index into the related string table
        uint8_t  info;
        uint8_t  reserved;
        uint16_t SecIdx;
        uint64_t Value;        // Value (Address) of the symbol
        uint64_t Size;         // Size of the object referenced by the symbol
} Elf64SymEntry;


#endif // Include guard;