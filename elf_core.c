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

#include "elf_core.h"

#define CTX(ctx) ((InternalElfCtx *)(ctx))

typedef struct
{
        uint8_t initialized;
        EiClass Class;
        EiData Endianness;
        void *UserCtx; // NULL if unused.
        elf_read_callback Callback;
        uint64_t ProHeadOff; // Offset of program header table in the file
        uint64_t SecHeadOff; // Offset of section header table in the file
        uint16_t PHEntrySize;    // Size of one entry in the program header table
        uint16_t PHEntryNum;     // Number of entries in the program header table
        uint16_t SHEntrySize;    // Size of one entry in the section header table
        uint16_t SHEntryNum;     // Number of entries in the section header table
        uint16_t SecNameStrIndx; // Index of the entry in the section header table that points to the section names.
} InternalElfCtx;

//_Static_assert(sizeof(InternalElfCtx) <= ELF_CTX_SIZE, "ELF_CTX_SIZE too small"); gives problems with rust compilation.

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
        ElfInfo;
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
        ElfInfo;
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
        return ((v >> 24) & 0x000000FF) |
               ((v >> 8)  & 0x0000FF00) |
               ((v << 8)  & 0x00FF0000) |
               ((v << 24) & 0xFF000000);
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
    
/** Read functions with automatic host-endian detection */
static inline uint16_t read_16(const uint16_t *data, EiData elf_endianness) {
        uint16_t v = *data;
        if (elf_endianness != host_endianness() && elf_endianness != DATA_NONE) {
            v = swap16(v);
        }
        return v;
}
    
static inline uint32_t read_32(const uint32_t *data, EiData elf_endianness) {
        uint32_t v = *data;
        if (elf_endianness != host_endianness() && elf_endianness != DATA_NONE) {
            v = swap32(v);
        }
        return v;
}
    
static inline uint64_t read_64(const uint64_t *data, EiData elf_endianness) {
        uint64_t v = *data;
        if (elf_endianness != host_endianness() && elf_endianness != DATA_NONE) {
            v = swap64(v);
        }
        return v;
}

ElfResult elf_init(void *user_ctx, elf_read_callback callback, ElfCtx *ctx)
{
        uint8_t magic_buff[16];
        ElfResult res = ELF_OK;
        uint8_t header_buff[sizeof(Elf64Header)] = {0};

        CTX(ctx)->initialized = false;

        /* Initialize base fields of the context */
        if ((callback == NULL))
                return ELF_BAD_ARG;

        CTX(ctx)->Callback = callback;
        CTX(ctx)->UserCtx = user_ctx;

        /* Parse identification section */
        res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, 16, magic_buff);
        if (res)
                return res;

        /* Validate magic */
        if (magic_buff[0] != 0x7f)
                return ELF_BAD_MAGIC;
        if (magic_buff[1] != 'E')
                return ELF_BAD_MAGIC;
        if (magic_buff[2] != 'L')
                return ELF_BAD_MAGIC;
        if (magic_buff[3] != 'F')
                return ELF_BAD_MAGIC;

        if (magic_buff[6] != 1)
                return ELF_BAD_VERSION;

        CTX(ctx)->Class = (EiClass)magic_buff[4];
        CTX(ctx)->Endianness = (EiData)magic_buff[5];

        if (CTX(ctx)->Class == CLASS_NONE)
                return ELF_BAD_CLASS;
        if (CTX(ctx)->Endianness == DATA_NONE)
                return ELF_BAD_ENDIANNESS;

        /* Parse header fields to cache values for future library calls */
        if (CTX(ctx)->Class == CLASS_32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf32Header), header_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf64Header), header_buff);
        }
        if (!res)
        {
                if (CTX(ctx)->Class == CLASS_32)
                {

                        CTX(ctx)->ProHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->ProHeadOff, CTX(ctx)->Endianness);
                        CTX(ctx)->SecHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->SecHeadOff, CTX(ctx)->Endianness);
                        CTX(ctx)->PHEntrySize = read_16(&((Elf32Header *)header_buff)->PHEntrySize, CTX(ctx)->Endianness);
                        CTX(ctx)->PHEntryNum  = read_16(&((Elf32Header *)header_buff)->PHEntryNum,  CTX(ctx)->Endianness);
                        CTX(ctx)->SHEntrySize = read_16(&((Elf32Header *)header_buff)->SHEntrySize, CTX(ctx)->Endianness);
                        CTX(ctx)->SHEntryNum  = read_16(&((Elf32Header *)header_buff)->SHEntryNum,  CTX(ctx)->Endianness);
                        CTX(ctx)->SecNameStrIndx = read_16(&((Elf32Header *)header_buff)->SecNameStrIndx, CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        CTX(ctx)->ProHeadOff  = read_64(&((Elf64Header *)header_buff)->ProHeadOff,  CTX(ctx)->Endianness);
                        CTX(ctx)->SecHeadOff  = read_64(&((Elf64Header *)header_buff)->SecHeadOff,  CTX(ctx)->Endianness);
                        CTX(ctx)->PHEntrySize = read_16(&((Elf64Header *)header_buff)->PHEntrySize, CTX(ctx)->Endianness);
                        CTX(ctx)->PHEntryNum  = read_16(&((Elf64Header *)header_buff)->PHEntryNum,  CTX(ctx)->Endianness);
                        CTX(ctx)->SHEntrySize = read_16(&((Elf64Header *)header_buff)->SHEntrySize, CTX(ctx)->Endianness);
                        CTX(ctx)->SHEntryNum  = read_16(&((Elf64Header *)header_buff)->SHEntryNum,  CTX(ctx)->Endianness);
                        CTX(ctx)->SecNameStrIndx = read_16(&((Elf64Header *)header_buff)->SecNameStrIndx, CTX(ctx)->Endianness);
                }
        }

        CTX(ctx)->initialized = true;
        return res;
}

ElfResult parse_header(const ElfCtx *ctx, ElfHeader *header)
{
        ElfResult res = ELF_OK;
        uint8_t header_buff[sizeof(Elf64Header)] = {0};

        if ((ctx == NULL) || (header == NULL))
                return ELF_BAD_ARG;


        if (!CTX(ctx)->initialized)
                return ELF_UNINIT;

        if (CTX(ctx)->Class == CLASS_32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf32Header), header_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf64Header), header_buff);
        }
        if (!res)
        {
                header->EI_Class       = ((Elf64Header *)header_buff)->EI_Class;
                header->EI_Data        = ((Elf64Header *)header_buff)->EI_Data;
                header->EI_Version     = ((Elf64Header *)header_buff)->EI_Version;
                header->EI_OS_ABI      = ((Elf64Header *)header_buff)->EI_OS_ABI;
                header->EI_ABI_Version = ((Elf64Header *)header_buff)->EI_ABI_Version;
                header->Type           = read_16(&((Elf64Header *)header_buff)->Type,    CTX(ctx)->Endianness);
                header->Machine        = read_16(&((Elf64Header *)header_buff)->Machine, CTX(ctx)->Endianness);
                header->Version        = read_32(&((Elf64Header *)header_buff)->Version, CTX(ctx)->Endianness);

                if (CTX(ctx)->Class == CLASS_32)
                {
                        header->Entry       = (uint64_t)read_32(&((Elf32Header *)header_buff)->Entry,      CTX(ctx)->Endianness);
                        header->ProHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->ProHeadOff, CTX(ctx)->Endianness);
                        header->SecHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->SecHeadOff, CTX(ctx)->Endianness);
                        header->Flags       = read_32(&((Elf32Header *)header_buff)->Flags,       CTX(ctx)->Endianness);
                        header->HeadSize    = read_16(&((Elf32Header *)header_buff)->HeadSize,    CTX(ctx)->Endianness);
                        header->PHEntrySize = read_16(&((Elf32Header *)header_buff)->PHEntrySize, CTX(ctx)->Endianness);
                        header->PHEntryNum  = read_16(&((Elf32Header *)header_buff)->PHEntryNum,  CTX(ctx)->Endianness);
                        header->SHEntrySize = read_16(&((Elf32Header *)header_buff)->SHEntrySize, CTX(ctx)->Endianness);
                        header->SHEntryNum  = read_16(&((Elf32Header *)header_buff)->SHEntryNum,  CTX(ctx)->Endianness);
                        header->SecNameStrIndx = read_16(&((Elf32Header *)header_buff)->SecNameStrIndx, CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        header->Entry       = read_64(&((Elf64Header *)header_buff)->Entry,       CTX(ctx)->Endianness);
                        header->ProHeadOff  = read_64(&((Elf64Header *)header_buff)->ProHeadOff,  CTX(ctx)->Endianness);
                        header->SecHeadOff  = read_64(&((Elf64Header *)header_buff)->SecHeadOff,  CTX(ctx)->Endianness);
                        header->Flags       = read_32(&((Elf64Header *)header_buff)->Flags,       CTX(ctx)->Endianness);
                        header->HeadSize    = read_16(&((Elf64Header *)header_buff)->HeadSize,    CTX(ctx)->Endianness);
                        header->PHEntrySize = read_16(&((Elf64Header *)header_buff)->PHEntrySize, CTX(ctx)->Endianness);
                        header->PHEntryNum  = read_16(&((Elf64Header *)header_buff)->PHEntryNum,  CTX(ctx)->Endianness);
                        header->SHEntrySize = read_16(&((Elf64Header *)header_buff)->SHEntrySize, CTX(ctx)->Endianness);
                        header->SHEntryNum  = read_16(&((Elf64Header *)header_buff)->SHEntryNum,  CTX(ctx)->Endianness);
                        header->SecNameStrIndx = read_16(&((Elf64Header *)header_buff)->SecNameStrIndx, CTX(ctx)->Endianness);
                }
        }

        return res;
}

uint16_t get_section_count(const ElfCtx *ctx)
{
        /* This function is designed to facilitate iterating over the sections,
        returning an error would make this fucntion useless as it would require
        more setup than what it is trying to remove. Returning 0 makes the 
        iteration empty and thus isn't a big safety compromise. */
        if (ctx == NULL)
                return 0;
        
        return CTX(ctx)->SHEntryNum;     
}

uint16_t get_program_header_count(const ElfCtx *ctx)
{
        /* This function is designed to facilitate iterating over the program headers,
        returning an error would make this fucntion useless as it would require
        more setup than what it is trying to remove. Returning 0 makes the 
        iteration empty and thus isn't a big safety compromise. */
        if (ctx == NULL)
                return 0;
        
        return CTX(ctx)->PHEntryNum;
}

ElfResult get_section_header(const ElfCtx *ctx, uint32_t idx, ElfSecHeader *sec_header)
{
        ElfResult res = ELF_OK;
        uint8_t sec_head_buff[sizeof(Elf64SecHeader)] = {0};

        if (!CTX(ctx)->initialized)
                return ELF_UNINIT;

        if (idx >= CTX(ctx)->SHEntryNum)
                return ELF_BAD_ARG;

        if (CTX(ctx)->Class == CLASS_32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->SecHeadOff) + idx * (CTX(ctx)->SHEntrySize), sizeof(Elf32SecHeader), sec_head_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->SecHeadOff) + idx * (CTX(ctx)->SHEntrySize), sizeof(Elf64SecHeader), sec_head_buff);
        }
        if(!res)
        {
                sec_header->NameIdx = read_32(&(((Elf64SecHeader *)sec_head_buff)->NameIdx), CTX(ctx)->Endianness);
                sec_header->Type    = read_32(&(((Elf64SecHeader *)sec_head_buff)->Type),    CTX(ctx)->Endianness);

                if (sec_header->Type > SECTION_OTHER)
                        sec_header->Type = SECTION_OTHER;
                
                if (CTX(ctx)->Class == CLASS_32)
                {
                        sec_header->Flags   = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->Flags),   CTX(ctx)->Endianness);
                        sec_header->Address = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->Address), CTX(ctx)->Endianness);
                        sec_header->Offset  = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->Offset),  CTX(ctx)->Endianness);
                        sec_header->Size    = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->Size),    CTX(ctx)->Endianness);
                        sec_header->Link    = read_32(&(((Elf32SecHeader *)sec_head_buff)->Link),    CTX(ctx)->Endianness);
                        sec_header->Info    = read_32(&(((Elf32SecHeader *)sec_head_buff)->Info),    CTX(ctx)->Endianness);
                        sec_header->Alignment = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->Alignment), CTX(ctx)->Endianness);
                        sec_header->EntrySize = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->EntrySize), CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        sec_header->Flags   = read_64(&(((Elf64SecHeader *)sec_head_buff)->Flags),   CTX(ctx)->Endianness);
                        sec_header->Address = read_64(&(((Elf64SecHeader *)sec_head_buff)->Address), CTX(ctx)->Endianness);
                        sec_header->Offset  = read_64(&(((Elf64SecHeader *)sec_head_buff)->Offset),  CTX(ctx)->Endianness);
                        sec_header->Size    = read_64(&(((Elf64SecHeader *)sec_head_buff)->Size),    CTX(ctx)->Endianness);
                        sec_header->Link    = read_32(&(((Elf64SecHeader *)sec_head_buff)->Link),    CTX(ctx)->Endianness);
                        sec_header->Info    = read_32(&(((Elf64SecHeader *)sec_head_buff)->Info),    CTX(ctx)->Endianness);
                        sec_header->Alignment = read_64(&(((Elf64SecHeader *)sec_head_buff)->Alignment), CTX(ctx)->Endianness);
                        sec_header->EntrySize = read_64(&(((Elf64SecHeader *)sec_head_buff)->EntrySize), CTX(ctx)->Endianness);
                }
        }

        return res;
}

// internal fuction, does not perform argument checks
static ElfResult internal_get_str_from_offset(const ElfCtx *ctx, uint64_t offset, uint8_t *buff, uint16_t len)
{
        ElfResult res = ELF_OK;
        uint16_t i = 0;

        while (i < (len)) {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, offset + i, 1, &buff[i]);
                if (res)
                        return res;

                if (buff[i] == '\0')
                        break;

                i++;
        }

        // Detect overflow (no null terminator found)
        if (buff[i] != '\0')
                return ELF_BUFFER_OVERFLOW;

        return ELF_OK;
}

ElfResult get_section_name(const ElfCtx *ctx, const ElfSecHeader *sec_header, uint8_t *buff, uint16_t len)
{
        ElfSecHeader str_sec_hdr;
        ElfResult res = ELF_OK; 

        if ((ctx == NULL) || (sec_header == NULL) || (buff == NULL) || (len == 0))
                return ELF_BAD_ARG;

        if (!CTX(ctx)->initialized)
                return ELF_UNINIT;

        res = get_section_header(ctx, CTX(ctx)->SecNameStrIndx, &str_sec_hdr);
        if (res)
                return res;

        uint64_t offset = str_sec_hdr.Offset + sec_header->NameIdx;
        return internal_get_str_from_offset(ctx, offset, buff, len);
}

ElfResult get_program_header(const ElfCtx *ctx, uint32_t idx, ElfProHeader *prog_header)
{
        ElfResult res = ELF_OK;
        uint8_t prog_head_buff[sizeof(Elf64ProHeader)] = {0};

        if (!CTX(ctx)->initialized)
                return ELF_UNINIT;

        if (idx >= CTX(ctx)->PHEntryNum)
                return ELF_BAD_ARG;


        if (CTX(ctx)->Class == CLASS_32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->ProHeadOff) + idx * (CTX(ctx)->PHEntrySize), sizeof(Elf32SecHeader), prog_head_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->ProHeadOff) + idx * (CTX(ctx)->PHEntrySize), sizeof(Elf64SecHeader), prog_head_buff);
        }
        if(!res)
        {
                prog_header->Type    = read_32(&(((Elf64ProHeader *)prog_head_buff)->Type),    CTX(ctx)->Endianness);
                
                if (CTX(ctx)->Class == CLASS_32)
                {
                        prog_header->Flags      = read_32(&(((Elf32ProHeader *)prog_head_buff)->Flags),      CTX(ctx)->Endianness);
                        prog_header->Offset     = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->Offset),     CTX(ctx)->Endianness);
                        prog_header->PhyAddress = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->PhyAddress), CTX(ctx)->Endianness);
                        prog_header->VirAddress = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->VirAddress), CTX(ctx)->Endianness);
                        prog_header->FileSize   = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->FileSize),   CTX(ctx)->Endianness);
                        prog_header->MemSize    = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->MemSize),    CTX(ctx)->Endianness);
                        prog_header->Alignment  = (uint64_t) read_32(&(((Elf32ProHeader *)prog_head_buff)->Alignment),  CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        prog_header->Flags   = read_32(&(((Elf64ProHeader *)prog_head_buff)->Flags),  CTX(ctx)->Endianness);
                        prog_header->Offset  = read_64(&(((Elf64ProHeader *)prog_head_buff)->Offset), CTX(ctx)->Endianness);
                        prog_header->PhyAddress = read_64(&(((Elf64ProHeader *)prog_head_buff)->PhyAddress), CTX(ctx)->Endianness);
                        prog_header->VirAddress = read_64(&(((Elf64ProHeader *)prog_head_buff)->VirAddress), CTX(ctx)->Endianness);
                        prog_header->FileSize   = read_64(&(((Elf64ProHeader *)prog_head_buff)->FileSize),   CTX(ctx)->Endianness);
                        prog_header->MemSize    = read_64(&(((Elf64ProHeader *)prog_head_buff)->MemSize),    CTX(ctx)->Endianness);
                        prog_header->Alignment  = read_64(&(((Elf64ProHeader *)prog_head_buff)->Alignment),  CTX(ctx)->Endianness);
                }       
        }

        return res;
}

uint32_t get_symbol_count(const ElfCtx *ctx, const ElfSecHeader *sym_tab)
{
        /* This function is designed to facilitate iterating over the symbols,
        returning an error would make this fucntion useless as it would require
        more setup than what it is trying to remove. Returning 0 makes the 
        iteration empty and thus isn't a big safety compromise. */
        if((ctx == NULL) ||(sym_tab == NULL) || (sym_tab->EntrySize == 0))
                return 0;
        
        return sym_tab->Size/sym_tab->EntrySize;

}

ElfResult get_symbol_entry(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint32_t idx, ElfSymTabEntry *sym)
{
        ElfResult res = ELF_OK;
        uint8_t sym_buff[sizeof(Elf64SymEntry)] = {0};

        if (!CTX(ctx)->initialized)
                return ELF_UNINIT;

        if((sym_tab == NULL)||(sym == NULL))
                return ELF_BAD_ARG;

        if (CTX(ctx)->Class == CLASS_32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (sym_tab->Offset) + idx * (sym_tab->EntrySize), sizeof(Elf32SymEntry), sym_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (sym_tab->Offset) + idx * (sym_tab->EntrySize), sizeof(Elf64SymEntry), sym_buff);
        }
        if(!res)
        {
                sym->NameIdx = read_32(&(((Elf64SymEntry *)sym_buff)->NameIdx), CTX(ctx)->Endianness);
                
                if (CTX(ctx)->Class == CLASS_32)
                {
                        sym->Type    = (((Elf32SymEntry *)sym_buff)->SecIdx) & 0x0F;
                        sym->Binding = (((Elf32SymEntry *)sym_buff)->SecIdx) >> 4;
                        sym->SecIdx  = read_16(&(((Elf32SymEntry *)sym_buff)->SecIdx), CTX(ctx)->Endianness);
                        sym->Value   = (uint64_t) read_32(&(((Elf32SymEntry *)sym_buff)->Value), CTX(ctx)->Endianness);
                        sym->Size    = (uint64_t) read_32(&(((Elf32SymEntry *)sym_buff)->Size),  CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        sym->Type    = (((Elf64SymEntry *)sym_buff)->SecIdx) & 0x0F;
                        sym->Binding = (((Elf64SymEntry *)sym_buff)->SecIdx) >> 4;
                        sym->SecIdx  = read_16(&(((Elf64SymEntry *)sym_buff)->SecIdx), CTX(ctx)->Endianness);
                        sym->Value   = read_64(&(((Elf64SymEntry *)sym_buff)->Value),  CTX(ctx)->Endianness);
                        sym->Size    = read_64(&(((Elf64SymEntry *)sym_buff)->Size),   CTX(ctx)->Endianness);
                }       
        }

        return res;
}

ElfResult get_symbol_name(const ElfCtx *ctx, uint32_t str_tab_idx, const ElfSymTabEntry *sym, uint8_t *buff, uint16_t len)
{
        // parameter checks for ctx, buff, str_tab_idx, buff, and len are already performed by get_str_from_table
        if((sym == NULL) || (str_tab_idx == 0))
                return ELF_BAD_ARG;
        
        return get_str_from_table(ctx, str_tab_idx, sym->NameIdx, buff, len);
}

ElfResult get_symbol_by_addr_exact( const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint64_t addr, ElfSymTabEntry *sym)
{
        ElfResult res = ELF_OK;
        // Already checks that ctx & sym_tab are valid.
        uint32_t sym_cnt = get_symbol_count(ctx, sym_tab);

        if (sym_cnt == 0)
                return ELF_BAD_ARG;
        
        // Skip NULL symbol
        for (uint32_t i = 1; i < sym_cnt; i++)
        {
                // Checks that sym is valid.
                res = get_symbol_entry(ctx, sym_tab, i, sym);
                if (res != ELF_OK)
                        return res;

                // Undefined symbols have no address
                if (sym->SecIdx == SHN_UNDEF)
                        continue;
                // Only consider function and object symbols
                if ((sym->Type != SYM_FUNC) && (sym->Type != SYM_OBJECT))
                        continue;

                if (sym->Value == addr)
                        return ELF_OK;
        }
        return ELF_NOT_FOUND;
}

ElfResult get_symbol_by_addr_range(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint64_t addr, ElfSymTabEntry *sym)
{
        ElfResult res = ELF_OK;
        // Already checks that ctx & sym_tab are valid.
        uint32_t sym_cnt = get_symbol_count(ctx, sym_tab);

        if (sym_cnt == 0)
                return ELF_BAD_ARG;
        
        // Skip NULL symbol
        for (uint32_t i = 1; i < sym_cnt; i++)
        {
                // Checks that sym is valid.
                res = get_symbol_entry(ctx, sym_tab, i, sym);
                if (res != ELF_OK)
                        return res;

                // Undefined symbols have no address
                if (sym->SecIdx == SHN_UNDEF)
                        continue;
                // Check only for function and object symbols
                if (addr >= sym->Value && addr <  sym->Value + sym->Size)
                        return ELF_OK;
        }
        return ELF_NOT_FOUND;       
}

ElfResult get_str_from_table(const ElfCtx *ctx, uint32_t sec_idx, uint32_t str_idx, uint8_t *buff, uint16_t len)
{
        ElfResult res = ELF_OK;
        ElfSecHeader str_tab;

        // parameter checks for ctx and sec_idx are already performed by get_section_header
        res = get_section_header(ctx, sec_idx, &str_tab);

        if(res)
                return res;

        if((str_tab.Type != SECTION_STRTAB) || (str_tab.Size <= str_idx) || (buff == NULL) || (len == 0))
                return ELF_BAD_ARG;
        
        uint64_t offset = str_tab.Offset + str_idx;
        return internal_get_str_from_offset(ctx, offset, buff, len);
}