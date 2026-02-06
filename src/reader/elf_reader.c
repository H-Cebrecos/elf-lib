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

#include <stddef.h>
#include "common/elf_core.h"
#include "common/elf_repr.h"
#include "common/elf_common.h"
#include "elf_reader.h"

#define CTX(ctx) ((InternalElfCtx *)(ctx))

typedef struct
{
        uint8_t initialized;
        EiClass Class;
        EiData Endianness;
        void *UserCtx; // NULL if unused.
        elf_read_callback Callback;
        ElfHeader Hdr;
} InternalElfCtx;

_Static_assert(sizeof(InternalElfCtx) <= ELF_CTX_SIZE, "ELF_CTX_SIZE too small");
    
/** Read functions with automatic host-endian detection */
static inline uint16_t read_16(const uint16_t *data, EiData elf_endianness) {
        uint16_t v = *data;
        if ((elf_endianness != host_endianness()) && (elf_endianness != ELFDATANONE)) {
            v = swap16(v);
        }
        return v;
}
    
static inline uint32_t read_32(const uint32_t *data, EiData elf_endianness) {
        uint32_t v = *data;
        if ((elf_endianness != host_endianness()) && (elf_endianness != ELFDATANONE)) {
            v = swap32(v);
        }
        return v;
}
    
static inline uint64_t read_64(const uint64_t *data, EiData elf_endianness) {
        uint64_t v = *data;
        if ((elf_endianness != host_endianness()) && (elf_endianness != ELFDATANONE)) {
            v = swap64(v);
        }
        return v;
}

/** Helper expression to reduce repetition */
static inline ElfResult validate_ctx(const ElfCtx *ctx)
{
        if ((ctx == NULL) || !(CTX(ctx)->initialized))
        {
                return ELF_UNINIT;
        }
        
        return ELF_OK;
}

ElfResult elf_init(void *user_ctx, elf_read_callback callback, ElfCtx *ctx)
{
        ElfResult res = ELF_OK;
        ElfInfo hdr_info;
        uint8_t header_buff[sizeof(Elf64Header)] = {0};
        ElfSecHeader null_sec;

        if (ctx == NULL)
        {
                return ELF_BAD_ARG;
        }

        CTX(ctx)->initialized = false;

        /* Initialize base fields of the context */
        if (callback == NULL)
        {
                return ELF_BAD_ARG;
        }

        CTX(ctx)->Callback = callback;
        CTX(ctx)->UserCtx = user_ctx;

        /* Parse identification header */
        res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(hdr_info), (void *)&hdr_info);
        if (res)
        {
                return res;
        }

        /* Validate magic */
        if (hdr_info.Magic[0] != 0x7f
         || hdr_info.Magic[1] != 'E'
         || hdr_info.Magic[2] != 'L'
         || hdr_info.Magic[3] != 'F')
        {
               return ELF_BAD_MAGIC;
        }

        if (hdr_info.EI_Version != EV_CURRENT)
        {
                return ELF_BAD_VERSION;
        }

        CTX(ctx)->Class= (EiClass)hdr_info.EI_Class;
        if (hdr_info.EI_Class == ELFCLASSNONE || (hdr_info.EI_Class != ELFCLASS32 && hdr_info.EI_Class != ELFCLASS64))
        {
                return ELF_BAD_CLASS;
        }
        
        CTX(ctx)->Endianness = (EiData)hdr_info.EI_Data;
        if (hdr_info.EI_Data == ELFDATANONE || (hdr_info.EI_Data != ELFDATA2LSB && hdr_info.EI_Data != ELFDATA2MSB))
        {
                return ELF_BAD_ENDIANNESS;
        }

        CTX(ctx)->Hdr.EI_OS_ABI = (ElfABI)hdr_info.EI_OS_ABI;
        CTX(ctx)->Hdr.EI_ABI_Version = hdr_info.EI_ABI_Version;

        /* Parse header fields to cache values for future library calls */
        if (CTX(ctx)->Class == ELFCLASS32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf32Header), header_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, 0, sizeof(Elf64Header), header_buff);
        }
        if (!res)
        {
                CTX(ctx)->Hdr.Type    = read_16(&((Elf32Header *)header_buff)->e_type,    CTX(ctx)->Endianness);
                CTX(ctx)->Hdr.Machine = read_16(&((Elf32Header *)header_buff)->e_machine, CTX(ctx)->Endianness);
                CTX(ctx)->Hdr.Version = read_32(&((Elf32Header *)header_buff)->e_version, CTX(ctx)->Endianness);

                if (CTX(ctx)->Hdr.Version != EV_CURRENT)
                {
                        return ELF_BAD_VERSION;
                }

                if (CTX(ctx)->Class == ELFCLASS32)
                {

                        CTX(ctx)->Hdr.Entry       = (uint64_t)read_32(&((Elf32Header *)header_buff)->e_entry,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.ProHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->e_phoff,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SecHeadOff  = (uint64_t)read_32(&((Elf32Header *)header_buff)->e_shoff,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.HeadSize    =           read_16(&((Elf32Header *)header_buff)->e_ehsize,    CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.PHEntrySize =           read_16(&((Elf32Header *)header_buff)->e_phentsize, CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.PHEntryNum  =           read_16(&((Elf32Header *)header_buff)->e_phnum,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SHEntrySize =           read_16(&((Elf32Header *)header_buff)->e_shentsize, CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SHEntryNum  =           read_16(&((Elf32Header *)header_buff)->e_shnum,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SecStrIndx  =           read_16(&((Elf32Header *)header_buff)->e_shstrndx,  CTX(ctx)->Endianness);

                        if (CTX(ctx)->Hdr.HeadSize != sizeof(Elf32Header))
                        {
                                return ELF_BAD_SIZE;
                        }

                        if (CTX(ctx)->Hdr.PHEntryNum &&
                            CTX(ctx)->Hdr.PHEntrySize != sizeof(Elf32ProHeader))
                        {

                                return ELF_BAD_SIZE;
                        }

                        if (CTX(ctx)->Hdr.SHEntryNum &&
                            CTX(ctx)->Hdr.SHEntrySize != sizeof(Elf32SecHeader))
                        {

                                return ELF_BAD_SIZE;
                        }
                }
                else // 64 bit
                {
                        CTX(ctx)->Hdr.Entry       = read_64(&((Elf64Header *)header_buff)->e_entry,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.ProHeadOff  = read_64(&((Elf64Header *)header_buff)->e_phoff,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SecHeadOff  = read_64(&((Elf64Header *)header_buff)->e_shoff,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.HeadSize    = read_16(&((Elf64Header *)header_buff)->e_ehsize,    CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.PHEntrySize = read_16(&((Elf64Header *)header_buff)->e_phentsize, CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.PHEntryNum  = read_16(&((Elf64Header *)header_buff)->e_phnum,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SHEntrySize = read_16(&((Elf64Header *)header_buff)->e_shentsize, CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SHEntryNum  = read_16(&((Elf64Header *)header_buff)->e_shnum,     CTX(ctx)->Endianness);
                        CTX(ctx)->Hdr.SecStrIndx  = read_16(&((Elf64Header *)header_buff)->e_shstrndx,  CTX(ctx)->Endianness);

                        if (CTX(ctx)->Hdr.HeadSize != sizeof(Elf64Header))
                        {
                                return ELF_BAD_SIZE;
                        }
                        
                        if (CTX(ctx)->Hdr.PHEntryNum &&
                            CTX(ctx)->Hdr.PHEntrySize != sizeof(Elf64ProHeader))
                        {

                                return ELF_BAD_SIZE;
                        }

                        if (CTX(ctx)->Hdr.SHEntryNum &&
                            CTX(ctx)->Hdr.SHEntrySize != sizeof(Elf64SecHeader))
                        {

                                return ELF_BAD_SIZE;
                        }
                }

                if (CTX(ctx)->Hdr.PHEntryNum != 0 && CTX(ctx)->Hdr.ProHeadOff == 0){
                        return ELF_BAD_HEADER;
                }
                if (CTX(ctx)->Hdr.SHEntryNum != 0 && CTX(ctx)->Hdr.SecHeadOff == 0){
                        return ELF_BAD_HEADER;
                }

                /* detect special indexes and get the proper values for the fields. */
                if (CTX(ctx)->Hdr.SHEntryNum == SHN_UNDEF || CTX(ctx)->Hdr.SecStrIndx == SHN_XINDEX)
                {
                        if (CTX(ctx)->Hdr.SecHeadOff == 0)
                        {
                                return ELF_BAD_HEADER;
                        }

                        res = get_section_header(ctx, 0, &null_sec);
                        if (res)
                        {
                                return res;
                        }

                        if (null_sec.Type != SHT_NULL)
                        {
                                return ELF_BAD_FORMAT;
                        }

                        if (CTX(ctx)->Hdr.SHEntryNum == SHN_UNDEF)
                        {
                                CTX(ctx)->Hdr.SHEntryNum = null_sec.Size;
                        }

                        if (CTX(ctx)->Hdr.SecStrIndx == SHN_XINDEX)
                        {
                                CTX(ctx)->Hdr.SecStrIndx = null_sec.Link;
                        }
                }
        }

        CTX(ctx)->initialized = true;
        return res;
}

ElfResult get_header(const ElfCtx *ctx, ElfHeader *header)
{
        ElfResult res = ELF_OK;

        if (validate_ctx(ctx))
        {
                return ELF_UNINIT;
        }
        
        if(header == NULL)
        {
                return ELF_BAD_ARG;
        }

        *header = CTX(ctx)->Hdr;

        return res;
}

uint16_t get_section_count(const ElfCtx *ctx)
{
        /* This function is designed to facilitate iterating over the sections,
        returning an error would make this fucntion useless as it would require
        more setup than what it is trying to remove. Returning 0 makes the 
        iteration empty and thus isn't a big safety compromise. */
        if (validate_ctx(ctx))
                return 0;
        
        return CTX(ctx)->Hdr.SHEntryNum;     
}

uint16_t get_program_header_count(const ElfCtx *ctx)
{
        /* This function is designed to facilitate iterating over the program headers,
        returning an error would make this fucntion useless as it would require
        more setup than what it is trying to remove. Returning 0 makes the 
        iteration empty and thus isn't a big safety compromise. */
        if (validate_ctx(ctx))
                return 0;
        
        return CTX(ctx)->Hdr.PHEntryNum;
}

ElfResult get_section_header(const ElfCtx *ctx, uint32_t idx, ElfSecHeader *sec_header)
{
        ElfResult res = ELF_OK;
        uint8_t sec_head_buff[sizeof(Elf64SecHeader)] = {0};

        if (validate_ctx(ctx))
        {
                return ELF_UNINIT;
        }

        if (sec_header == NULL)
        {
                return ELF_BAD_ARG;
        }

        if (idx >= CTX(ctx)->Hdr.SHEntryNum
        || CTX(ctx)->Hdr.SHEntryNum == 0)
        {
                return ELF_BAD_INDX;
        }

        if (CTX(ctx)->Class == ELFCLASS32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->Hdr.SecHeadOff) + idx * (CTX(ctx)->Hdr.SHEntrySize), sizeof(Elf32SecHeader), sec_head_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->Hdr.SecHeadOff) + idx * (CTX(ctx)->Hdr.SHEntrySize), sizeof(Elf64SecHeader), sec_head_buff);
        }
        if(!res)
        {
                sec_header->NameIdx = read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_name), CTX(ctx)->Endianness);
                sec_header->Type    = read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_type), CTX(ctx)->Endianness);

                if (CTX(ctx)->Class == ELFCLASS32)
                {
                        sec_header->Flags     = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_flags),     CTX(ctx)->Endianness);
                        sec_header->Address   = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_addr),      CTX(ctx)->Endianness);
                        sec_header->Offset    = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_offset),    CTX(ctx)->Endianness);
                        sec_header->Size      = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_size),      CTX(ctx)->Endianness);
                        sec_header->Link      =            read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_link),      CTX(ctx)->Endianness);
                        sec_header->Info      =            read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_info),      CTX(ctx)->Endianness);
                        sec_header->Alignment = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_addralign), CTX(ctx)->Endianness);
                        sec_header->EntrySize = (uint64_t) read_32(&(((Elf32SecHeader *)sec_head_buff)->sh_entsize),   CTX(ctx)->Endianness);
                
                        /* Perform validation based on section type */
                        if ((sec_header->Type == SHT_RELA) && (sec_header->EntrySize != sizeof(Elf32Rela)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_REL) && (sec_header->EntrySize != sizeof(Elf32Rel)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_RELR) && (sec_header->EntrySize != sizeof(Elf32Relr)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_DYNSYM || sec_header->Type == SHT_SYMTAB) 
                        && (sec_header->EntrySize != sizeof(Elf32SymEntry)))
                        {
                                return ELF_BAD_SIZE;
                        }
                }
                else // 64 bit
                {
                        sec_header->Flags     = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_flags),     CTX(ctx)->Endianness);
                        sec_header->Address   = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_addr),      CTX(ctx)->Endianness);
                        sec_header->Offset    = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_offset),    CTX(ctx)->Endianness);
                        sec_header->Size      = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_size),      CTX(ctx)->Endianness);
                        sec_header->Link      = read_32(&(((Elf64SecHeader *)sec_head_buff)->sh_link),      CTX(ctx)->Endianness);
                        sec_header->Info      = read_32(&(((Elf64SecHeader *)sec_head_buff)->sh_info),      CTX(ctx)->Endianness);
                        sec_header->Alignment = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_addralign), CTX(ctx)->Endianness);
                        sec_header->EntrySize = read_64(&(((Elf64SecHeader *)sec_head_buff)->sh_entsize),   CTX(ctx)->Endianness);
                
                        /* Perform validation based on section type */
                        if ((sec_header->Type == SHT_RELA) && (sec_header->EntrySize != sizeof(Elf64Rela)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_REL) && (sec_header->EntrySize != sizeof(Elf64Rel)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_RELR) && (sec_header->EntrySize != sizeof(Elf64Relr)))
                        {
                                return ELF_BAD_SIZE;
                        }
                        if ((sec_header->Type == SHT_DYNSYM || sec_header->Type == SHT_SYMTAB) 
                        && (sec_header->EntrySize != sizeof(Elf64SymEntry)))
                        {
                                return ELF_BAD_SIZE;
                        }
                }
                
                if (((sec_header->Flags & SHF_COMPRESSED) && (sec_header->Flags & SHF_ALLOC))
                 || ((sec_header->Flags & SHF_COMPRESSED) &&  (sec_header->Type == SHT_NOBITS)))
                {
                        return ELF_BAD_FORMAT;
                }

                if (sec_header->Type == SHT_GROUP && CTX(ctx)->Hdr.Type != ET_REL)
                {
                        return ELF_BAD_FORMAT;
                        //NOTE: This implementation does not check that a group section appears before the other sections in the group.
                }
                //NOTE: This implementation does not check that a SYMTAB_SHNDX is pressent if a symbol tables contains an SHN_XINDEX entry.
        }

        return res;
}

// internal fuction, does not perform argument checks
static ElfResult internal_get_str_from_offset(const ElfCtx *ctx, uint64_t offset, uint8_t *buff, uint16_t len)
{
        uint16_t i = 0;
        
        while (i < (len)) {
                ElfResult res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, offset + i, 1, &buff[i]);
                if (res)
                {
                        return res;
                }

                if (buff[i] == '\0')
                {
                        break;
                }

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

        if (validate_ctx(ctx))
        {
                return ELF_UNINIT;
        }

        if ((sec_header == NULL) || (buff == NULL) || (len == 0))
        {
                return ELF_BAD_ARG;
        }

        res = get_section_header(ctx, CTX(ctx)->Hdr.SecStrIndx, &str_sec_hdr);
        if (res)
        {
                return res;
        }

        uint64_t offset = str_sec_hdr.Offset + sec_header->NameIdx;
        return internal_get_str_from_offset(ctx, offset, buff, len);
}

ElfResult get_section_by_name(const ElfCtx *ctx, const uint8_t *name, ElfSecHeader *sec)
{
        uint8_t sec_name[256];

        // Already checks that ctx is valid.
        uint32_t sec_cnt = get_section_count(ctx);

        if ((name == NULL) || (sec_cnt == 0))
        {
                return ELF_BAD_ARG;
        }
        
        // Skip NULL section
        for (uint32_t i = 1; i < sec_cnt; i++)
        {
                // already checks that sec is valid
                ElfResult res = get_section_header(ctx, i, sec);
                if (res != ELF_OK)
                {
                        return res;
                }

                res = get_section_name(ctx, sec, sec_name, sizeof(sec_name));
                if (res != ELF_OK)
                {
                        return res;
                }

                for (uint16_t j = 0; j < 256; j++)
                {

                        if (name[j] != sec_name[j])
                        {
                                break;
                        }

                        // equality already considered in the previous check
                        if ((name[j] == '\0'))
                        {
                                return ELF_OK;
                        }
                }
        }
        return ELF_NOT_FOUND;
}

//TODO: get_compression_header(section);

//TODO: from this point.

ElfResult get_program_header(const ElfCtx *ctx, uint32_t idx, ElfProHeader *prog_header)
{
        ElfResult res = ELF_OK;
        uint8_t prog_head_buff[sizeof(Elf64ProHeader)] = {0};

        if (validate_ctx(ctx))
                return ELF_UNINIT;

        if (idx >= CTX(ctx)->Hdr.PHEntryNum)
                return ELF_BAD_ARG;


        if (CTX(ctx)->Class == ELFCLASS32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->Hdr.ProHeadOff) + idx * (CTX(ctx)->Hdr.PHEntrySize), sizeof(Elf32SecHeader), prog_head_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (CTX(ctx)->Hdr.ProHeadOff) + idx * (CTX(ctx)->Hdr.PHEntrySize), sizeof(Elf64SecHeader), prog_head_buff);
        }
        if(!res)
        {
                prog_header->Type    = read_32(&(((Elf64ProHeader *)prog_head_buff)->Type),    CTX(ctx)->Endianness);
                
                if (CTX(ctx)->Class == ELFCLASS32)
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
        if(validate_ctx(ctx) ||(sym_tab == NULL) || (sym_tab->EntrySize == 0))
                return 0;
        
        return sym_tab->Size/sym_tab->EntrySize;

}

ElfResult get_symbol_entry(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint32_t idx, ElfSymTabEntry *sym)
{
        ElfResult res = ELF_OK;
        uint8_t sym_buff[sizeof(Elf64SymEntry)] = {0};

        if (validate_ctx(ctx))
                return ELF_UNINIT;

        if((sym_tab == NULL)||(sym == NULL))
                return ELF_BAD_ARG;

        if (CTX(ctx)->Class == ELFCLASS32)
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (sym_tab->Offset) + idx * (sym_tab->EntrySize), sizeof(Elf32SymEntry), sym_buff);
        }
        else
        {
                res = CTX(ctx)->Callback(CTX(ctx)->UserCtx, (sym_tab->Offset) + idx * (sym_tab->EntrySize), sizeof(Elf64SymEntry), sym_buff);
        }
        if(!res)
        {
                sym->NameIdx = read_32(&(((Elf64SymEntry *)sym_buff)->st_name), CTX(ctx)->Endianness);
                
                if (CTX(ctx)->Class == ELFCLASS32)
                {
                        sym->Type    = (((Elf32SymEntry *)sym_buff)->st_info) & 0x0F;
                        sym->Binding = (((Elf32SymEntry *)sym_buff)->st_info) >> 4;
                        sym->SecIdx  = read_16(&(((Elf32SymEntry *)sym_buff)->st_shndx), CTX(ctx)->Endianness);
                        sym->Value   = (uint64_t) read_32(&(((Elf32SymEntry *)sym_buff)->st_value), CTX(ctx)->Endianness);
                        sym->Size    = (uint64_t) read_32(&(((Elf32SymEntry *)sym_buff)->st_size),  CTX(ctx)->Endianness);
                }
                else // 64 bit
                {
                        sym->Type    = (((Elf64SymEntry *)sym_buff)->st_info) & 0x0F;
                        sym->Binding = (((Elf64SymEntry *)sym_buff)->st_info) >> 4;
                        sym->SecIdx  = read_16(&(((Elf64SymEntry *)sym_buff)->st_shndx), CTX(ctx)->Endianness);
                        sym->Value   = read_64(&(((Elf64SymEntry *)sym_buff)->st_value),  CTX(ctx)->Endianness);
                        sym->Size    = read_64(&(((Elf64SymEntry *)sym_buff)->st_size),   CTX(ctx)->Endianness);
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
        // Already checks that ctx & sym_tab are valid.
        uint32_t sym_cnt = get_symbol_count(ctx, sym_tab);

        if (sym_cnt == 0)
                return ELF_BAD_ARG;
        
        // Skip NULL symbol
        for (uint32_t i = 1; i < sym_cnt; i++)
        {
                // Checks that sym is valid.
                ElfResult res = get_symbol_entry(ctx, sym_tab, i, sym);
                if (res != ELF_OK)
                        return res;

                // Undefined symbols have no address
                if (sym->SecIdx == SHN_UNDEF)
                        continue;
                // Only consider function and object symbols
                if ((sym->Type != STT_FUNC) && (sym->Type != STT_OBJECT))
                        continue;

                if (sym->Value == addr)
                        return ELF_OK;
        }
        return ELF_NOT_FOUND;
}

ElfResult get_symbol_by_addr_range(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint64_t addr, ElfSymTabEntry *sym)
{
        // Already checks that ctx & sym_tab are valid.
        uint32_t sym_cnt = get_symbol_count(ctx, sym_tab);

        if (sym_cnt == 0U)
                return ELF_BAD_ARG;
        
        // Skip NULL symbol
        for (uint32_t i = 1; i < sym_cnt; i++)
        {
                // Checks that sym is valid.
                ElfResult res = get_symbol_entry(ctx, sym_tab, i, sym);
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

ElfResult get_symbol_by_name(const ElfCtx *ctx, const uint8_t *name, const ElfSecHeader *sym_tab, ElfSymTabEntry *sym)
{
        ElfResult res;
        uint8_t sym_name[256];

        // Already checks that ctx & sym_tab are valid.
        uint32_t sym_cnt = get_symbol_count(ctx, sym_tab);

        if ((name == NULL) || (sym_cnt == 0))
                return ELF_BAD_ARG;
        
        // Skip NULL symbol
        for (uint32_t i = 1; i < sym_cnt; i++)
        {
                // Checks that sym is valid.
                res = get_symbol_entry(ctx, sym_tab, i, sym);
                if (res != ELF_OK)
                        return res;

                res = get_symbol_name(ctx, sym_tab->Link, sym, sym_name, sizeof(sym_name));
                if (res != ELF_OK)
                        return res;

                for (uint16_t j = 0; j < 256; j++)
                {

                        if (name[j] != sym_name[j])
                                break;

                        // equality already considered in the previous check
                        if ((name[j] == '\0'))
                                return ELF_OK;
                }
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

        if((str_tab.Type != SHT_STRTAB) || (str_tab.Size <= str_idx) || (buff == NULL) || (len == 0))
                return ELF_BAD_ARG;
        
        uint64_t offset = str_tab.Offset + str_idx;
        return internal_get_str_from_offset(ctx, offset, buff, len);
}