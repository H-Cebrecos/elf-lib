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

typedef struct {
    const void *data;
    uint64_t size;
    uint64_t align;
} Chunk;

typedef struct
{
        char *name; //TODO: ensure that it is copied internally as promised in the API
        uint32_t type;
        uint64_t flags;
        uint64_t align;

        Chunk *data;
        uint64_t size;
        uint64_t capacity;

        uint32_t link;
        uint32_t info;
        uint64_t entsize;
} ElfWSection;
typedef ElfWSection* sec_hndl; 
//typedef struct
//{
//        uint32_t type;
//        uint32_t flags;
//        uint64_t align;
//
//        /* Filled during layout */
//        uint64_t offset;
//        uint64_t vaddr;
//        uint64_t paddr;
//        uint64_t filesz;
//        uint64_t memsz;
//
//        /* Which sections are covered */
//        size_t *section_indices;
//        size_t section_count;
//} ElfWSegment;

typedef struct {
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

        ElfWSection *Sections;
        size_t Section_count;
        size_t Section_cap;

        ElfWSegment *Segments;
        size_t Segment_count;
        size_t Segment_cap; // capacity

        ElfIOCallback Callback;
        void *UserCtx;
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
                ctx->Sections = NULL;
                ctx->Section_count = 0;
                ctx->Section_cap = 0;

                ctx->Segments = NULL;
                ctx->Segment_count = 0;
                ctx->Segment_cap = 0;

                res = ctx;
        }
        return res;
}


static inline void elfw_segment_destroy(ElfWSegment *s) { (void)s; } // TODO

void elfw_destroy(ElfwCtx *ctx)
{
        if (ctx == NULL)
                return;

        if (ctx->Head != NULL)
        {
                free(ctx->Head);
        }

        if (ctx->Sections != NULL)
        {
                for (size_t i = 0; i < ctx->Section_count; ++i)
                {
                        elfw_section_destroy(&ctx->Sections[i]);
                }
                free(ctx->Sections);
        }

        if (ctx->Segments != NULL)
        {
                for (size_t i = 0; i < ctx->Segment_count; ++i)
                {
                        elfw_segment_destroy(&ctx->Segments[i]);
                }
                free(ctx->Segments);
        }

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

ElfResult elfw_add_section(ElfwCtx *ctx, const char *name, const *ElfwSectionCreateInfo, sec_hndl * new_sec)
{
        if (ctx == NULL)
                return ELF_UNINIT;

        ElfWSection *sec = malloc(sizeof(*sec));
        if (sec == NULL)
                return ELF_NO_MEM;

        //TODO

        ctx->Sections = sec;

        return ELF_OK;
}

static inline void elfw_section_destroy(ElfWSection *s) { (void)s; }

ElfResult elfw_section_set_data(sec_hndl section, const void *data, uint64_t size, uint64_t align)
{
        //TODO
}

ElfResult elfw_section_append_data(sec_hndl section, const void *data, uint64_t size, uint64_t align)
{
        //TODO
}

uint64_t elfw_section_next_offset(ElfwCtx *ctx, sec_hndl section, uint64_t align)
{
        //TODO
}