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

#ifndef ELF_COMM_LIB
#define ELF_COMM_LIB

#include <stdint.h>
#include "elf_core.h"

typedef struct
{
        uint8_t Magic[4];   // 0x7f, E, L, F
        uint8_t EI_Class;   // 32 or 64 bit architecture
        uint8_t EI_Data;    // LSB or MSB architecture
        uint8_t EI_Version; // Always 1
        uint8_t EI_OS_ABI;       // Target Platform's ABI
        uint8_t EI_ABI_Version; // Target ABI Version
        uint8_t Pad[7];
        uint16_t Type;    // Type of ELF file
        uint16_t Machine; // Architecture
        uint32_t Version; // Always 1
} ElfInfo;

typedef struct
{
        ElfInfo  Info;
        uint32_t Entry;      // Entry point (virtual address)
        uint32_t ProHeadOff; // Offset of program header table in the file (bytes)
        uint32_t SecHeadOff; // Offset of section header table in the file (bytes)
        uint32_t Flags;
        uint16_t HeadSize;       // This header's size
        uint16_t PHEntrySize;    // Size of one entry in the program header table
        uint16_t PHEntryNum;     // Number of entries in the program header table
        uint16_t SHEntrySize;    // Size of one entry in the section header table
        uint16_t SHEntryNum;     // Number of entries in the section header table
        uint16_t SecNameStrIndx; // Index of the entry in the section table that points to the section names.
} Elf32Header;

typedef struct
{
        ElfInfo  Info;
        uint64_t Entry;      // Entry point (virtual address)
        uint64_t ProHeadOff; // Offset of program header table in the file
        uint64_t SecHeadOff; // Offset of section header table in the file
        uint32_t Flags;
        uint16_t HeadSize;       // This header's size
        uint16_t PHEntrySize;    // Size of one entry in the program header table
        uint16_t PHEntryNum;     // Number of entries in the program header table
        uint16_t SHEntrySize;    // Size of one entry in the section header table
        uint16_t SHEntryNum;     // Number of entries in the section header table
        uint16_t SecNameStrIndx; // Index of the entry in the section table that points to the section names.
} Elf64Header;

typedef struct
{
        uint32_t NameIdx; // Index into the section header string table section
        uint32_t Type;    // Type of section
        uint32_t Flags;
        uint32_t Address; // If the section is in the memory img of a process this is the first address
        uint32_t Offset;  // Section is stored at <offset> from the begining of this file
        uint32_t Size;    // Size of the section
        uint32_t Link;
        uint32_t Info;
        uint32_t Alignment; // Alignment constraints of Address field
        uint32_t EntrySize; // If the section is a table of fixed-size entries this is the entry size
} Elf32SecHeader;

typedef struct
{
        uint32_t NameIdx; // Index into the section header string table section
        uint32_t Type;    // Type of section
        uint64_t Flags;
        uint64_t Address; // If the section is in the memory img of a process this is the first address
        uint64_t Offset;  // Section is stored at <offset> from the begining of this file
        uint64_t Size;    // Size of the section
        uint32_t Link;
        uint32_t Info;
        uint64_t Alignment; // Alignment constraints of Address field
        uint64_t EntrySize; // If the section is a table of fixed-size entries this is the entry size
} Elf64SecHeader;

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

/** Detect host endianness at runtime */
static inline EiData host_endianness(void) {
        uint16_t x = 1;
        return (*(uint8_t *)&x) ? DATA_LSB : DATA_MSB;
}

/** Swap helpers */
static inline uint16_t swap16(uint16_t v) {
        return (v >> 8) | (v << 8);
}
    
static inline uint32_t swap32(uint32_t v) {
        return ((v >> 24) & 0x000000FFUL) |
               ((v >> 8)  & 0x0000FF00UL) |
               ((v << 8)  & 0x00FF0000UL) |
               ((v << 24) & 0xFF000000UL);
}
    
static inline uint64_t swap64(uint64_t v) {
        return ((v >> 56) & 0x00000000000000FFULL) |
               ((v >> 40) & 0x000000000000FF00ULL) |
               ((v >> 24) & 0x0000000000FF0000ULL) |
               ((v >> 8)  & 0x00000000FF000000ULL) |
               ((v << 8)  & 0x000000FF00000000ULL) |
               ((v << 24) & 0x0000FF0000000000ULL) |
               ((v << 40) & 0x00FF000000000000ULL) |
               ((v << 56) & 0xFF00000000000000ULL);
}

#endif // Include guard;