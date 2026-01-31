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

#include <stdlib.h>

#include "src/common/elf_core.h"
#include "src/common/elf_common.h"
#include "elf_writer.h"

typedef struct
{
        // TODO: think about this stuff.
        void (*write)(void *user, const void *buf, size_t size);
} ElfwSink;

// TODO(section-data-model):
// Sections are internally represented as a list of data chunks (scatter–gather),
// not as a single contiguous buffer.
//
// Rationale:
// - Avoids forcing large allocations (more no-std / embedded friendly).
// - Supports incremental construction (append-only workflows).
// - Naturally fits assemblers, debug info generation, and future linker-like use cases.
// - A contiguous buffer is a special case of a single chunk.
//
// Intended design:
// - Internally: section owns a dynamic list of { const void *data, size_t size } chunks.
// - sh_size is computed as the sum of all chunk sizes.
// - At write time, chunks are emitted sequentially to produce a contiguous section image.
//
// This keeps the simple cases trivial while preserving flexibility for advanced tools.
// Implementation deferred until section/segment layout logic is in place.
// TODO(alignment-model): think about this, this library is not a linker
// - Section alignment (sh_addralign) is enforced automatically by the layout engine.
// - Internal section layout uses explicit chunk alignment only.
// - Chunks may optionally specify an alignment; padding is inserted before the chunk.
// - No implicit or inferred alignment is performed inside sections.

/* Generic array list */
typedef struct
{
        void **data;
        uint32_t length;
        uint32_t capacity;
} ElfwVec;

typedef void (*ElfwElemDestroyFn)(void *elem);

static ElfResult elfw_vec_init(ElfwVec *v);
static void elfw_vec_destroy(ElfwVec *v, ElfwElemDestroyFn destroy);
static ElfResult elfw_vec_push(ElfwVec *v, void *elem);
static void *elfw_vec_get(const ElfwVec *v, uint32_t idx);

typedef struct
{
        const void *data; // not owned
        uint64_t size;
        uint64_t align;
} Chunk;

/* Internal section representation */
typedef struct ElfWSection ElfWSection;
struct ElfWSection
{
        char *Name;
        ElfSectionType Type;
        uint64_t Flags;
        uint64_t StartAddr;
        ElfWSection *Link;
        uint32_t Info;
        uint64_t Align;
        uint64_t EntrySize;

        ElfwVec Chunks;

        // Next free address after the data
        uint64_t Offset;
};

// typedef struct
//{
//         uint32_t type;
//         uint32_t flags;
//         uint64_t align;
//
//         /* Filled during layout */
//         uint64_t offset;
//         uint64_t vaddr;
//         uint64_t paddr;
//         uint64_t filesz;
//         uint64_t memsz;
//
//         /* Which sections are covered */
//         size_t *section_indices;
//         size_t section_count;
// } ElfWSegment;

typedef struct
{
        ElfWSection *section;
    uint64_t     sec_offset;   /* offset inside section */
    uint64_t     size;
    uint64_t     vaddr_align;  /* optional */
} ElfwSegMap;
typedef struct {
    //ElfWSegmentType type;
    uint32_t        flags;
    uint64_t        align;

    ElfwSegMap     *maps;
    size_t          map_count;
    size_t          map_cap;
} ElfWSegment;

struct ElfwCtx
{
        Elf64Header *Head;

        ElfwVec Sections;
        ElfwVec Segments;
};

ElfwCtx *elfw_create(void)
{
        ElfwCtx *res;
        ElfwCtx *ctx = calloc(1, sizeof(*ctx));
        if (!ctx)
        {
                res = NULL;
        }
        else
        {
                ctx->Head = NULL;
                elfw_vec_init(&(ctx->Segments));
                elfw_vec_init(&(ctx->Segments));

                res = ctx;
        }
        return res;
}

static inline void elfw_segment_destroy(ElfWSegment *s);
static inline void elfw_section_destroy(ElfWSection *s);

void elfw_destroy(ElfwCtx *ctx)
{
        if (ctx == NULL)
                return;

        if (ctx->Head != NULL)
        {
                free(ctx->Head);
        }

        elfw_vec_destroy(&(ctx->Sections), (ElfwElemDestroyFn)elfw_section_destroy);
        elfw_vec_destroy(&(ctx->Segments), (ElfwElemDestroyFn)elfw_segment_destroy);

        free(ctx);
}

ElfResult elfw_create_header(ElfwCtx *ctx, const ElfwHeaderCreateInfo *info)
{
        if (ctx == NULL)
                return ELF_UNINIT;

        Elf64Header *header = malloc(sizeof(*header));
        if (header == NULL)
                return ELF_NO_MEM;

        *header = (Elf64Header){
            .Info = {
                .Magic      = {0x7f, 'E', 'L', 'F'},
                .EI_Class   = info->Class,
                .EI_Data    = info->Endianness,
                .EI_Version = 1,
                .EI_OS_ABI  = info->Os_abi,
                .EI_ABI_Version = info->Abi_version,
                .Pad     = {0},
                .Type    = info->Type,
                .Machine = info->Machine,
                .Version = 1,
            },
            .Flags      = info->Flags,
            .Entry      = info->Entry,

            .HeadSize = sizeof(Elf64Header),
            .PHEntrySize = sizeof(Elf64ProHeader),
            .SHEntrySize = sizeof(Elf64SecHeader),

            /* Filled later */
            .SecHeadOff = 0,
            .ProHeadOff = 0,
            .PHEntryNum = 0,
            .SHEntryNum = 0,
            .SecNameStrIndx = 0,
        };

        ctx->Head = header;

        return ELF_OK;
}

ElfResult elfw_add_section(ElfwCtx *ctx, const ElfwSectionCreateInfo *info, sec_hndl *new_sec)
{
        *new_sec = NULL;

        if (ctx == NULL)
                return ELF_UNINIT;

        if ((info == NULL) || (new_sec == NULL))
                return ELF_BAD_ARG;

        /* Aligment and validity checks */
        {
                uint64_t align = info->Alignment;
                uint64_t addr = info->Address;

                if (align == 0)
                        return ELF_BAD_ARG;

                /* power to two */
                if ((align != 0) && ((align & (align - 1)) == 0))
                        return ELF_BAD_ARG;

                /* address is aligned */
                if ((addr != 0) && ((addr % align) != 0))
                        return ELF_BAD_ARG;

                /* Address only meaningful for allocatable sections */
                if ((!(info->Flags & SEC_FLAG_ALLOC)) && (addr != 0))
                        return ELF_BAD_ARG;

                /* Entry size must respect alignment */
                if ((info->EntrySize != 0) && ((info->EntrySize % align) != 0))
                        return ELF_BAD_ARG;

                switch (info->Type)
                {
                case SECTION_NULL:
                        /* NULL sections must not have payload semantics */
                        if ((addr != 0) || (info->EntrySize != 0))
                                return ELF_BAD_ARG;
                        break;

                case SECTION_STRTAB:
                        /* String tables have byte entries */
                        if ((info->EntrySize != 0) && (info->EntrySize != 1))
                                return ELF_BAD_ARG;
                        break;

                case SECTION_DYNSYM:
                case SECTION_SYMTAB:
                        if (ctx->Head != NULL)
                        {
                                uint64_t expected = (ctx->Head->Info.EI_Class == CLASS_32) ? sizeof(Elf32SymEntry) : sizeof(Elf64SymEntry);
                                if (info->EntrySize != expected)
                                        return ELF_BAD_ARG;
                        }

                        break;

                default:
                        /* Other types: no strict validation here */
                        // TODO: check what other validation could be done. This is the main validation point for genreating valid elfs.
                        break;
                }
        }

        ElfWSection *sec = malloc(sizeof(*sec));
        if (sec == NULL)
                return ELF_NO_MEM;

        /* copy name */
        {
                uint32_t len = 0;

                while (info->Name[len] != '\0')
                        len++;

                sec->Name = malloc(len + 1);
                if (sec->Name == NULL)
                        return ELF_NO_MEM;

                for (uint32_t i = 0; i <= len; i++)
                        sec->Name[i] = info->Name[i];
        }

        elfw_vec_init(&(sec->Chunks));

        sec->Type = info->Type;
        sec->Flags = info->Flags;
        sec->StartAddr = info->Address;
        sec->Link = info->Link;
        sec->Info = info->Info;
        sec->Align = info->Alignment;
        sec->EntrySize = info->EntrySize;

        sec->Offset = 0;

        elfw_vec_push(&(ctx->Sections), sec);

        *new_sec = sec;
        return ELF_OK;
}

static inline void elfw_section_destroy(ElfWSection *s)
{
        if (s == NULL)
                return;

        free(s->Name);
        elfw_vec_destroy(&(s->Chunks), (ElfwElemDestroyFn)free);
}

ElfResult elfw_section_set_data(sec_hndl section, const void *data, uint64_t size, uint64_t align)
{
        if (section == NULL)
                return ELF_UNINIT;

        elfw_vec_destroy(&(section->Chunks), (ElfwElemDestroyFn)free);
        section->Offset = 0;

        return elfw_section_append_data(section, data, size, align);
}

ElfResult elfw_section_append_data(sec_hndl section, const void *data, uint64_t size, uint64_t align)
{
        if (section == NULL)
                return ELF_UNINIT;

        if (data == NULL)
                return ELF_BAD_ARG;

        if (size == 0)
                return ELF_OK;

        Chunk *chk = malloc(sizeof(*chk));
        
        chk->data  = data;
        chk->size  = size;
        chk->align = align;
        
        elfw_vec_push(&(section->Chunks), &chk);

        section->Offset =elfw_section_next_offset(section, align) + size;

        return ELF_OK;
}

uint64_t elfw_section_next_offset(sec_hndl section, uint64_t align)
{
        // Power of 2 aligment:
        //  - Add (alignment - 1) to the current offset. This ensures that
        //    any remainder will carry the value past the next multiple.
        //  - Clear the lower bits corresponding to the alignment using bitwise AND with ~(alignment - 1),
        //    effectively rounding down to the nearest multiple of `alignment`.
        return (section->Offset + align - 1) & ~(align- 1);
}

static ElfResult elfw_vec_init(ElfwVec *v)
{
        if (v == NULL)
                return ELF_BAD_ARG;

        v->data = NULL;
        v->length = 0;
        v->capacity = 0;
        return ELF_OK;
}

static void elfw_vec_destroy(ElfwVec *v, ElfwElemDestroyFn destroy)
{
        if (v == NULL)
                return;

        if (destroy)
        {
                for (uint32_t i = 0; i < v->length; i++)
                        destroy(v->data[i]);
        }

        free(v->data);
        v->data = NULL;
        v->length = 0;
        v->capacity = 0;
}

static ElfResult elfw_vec_push(ElfwVec *v, void *elem)
{
        if ((v == NULL) || (elem == NULL))
                return ELF_BAD_ARG;

        if (v->length == v->capacity)
        {
                uint32_t new_cap = (v->capacity == 0) ? 4 : v->capacity * 2;

                void **new_data = realloc(v->data, new_cap * sizeof(void *));
                if (!new_data)
                        return ELF_NO_MEM;

                v->data = new_data;
                v->capacity = new_cap;
        }

        v->data[v->length++] = elem;
        return ELF_OK;
}

static void *elfw_vec_get(const ElfwVec *v, uint32_t idx)
{
        if ((v == NULL) || (idx >= v->length))
                return NULL;

        return v->data[idx];
}