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

typedef struct
{
        char *name;
        uint32_t type;
        uint64_t flags;
        uint64_t align;

        uint8_t *data;
        uint64_t size;

        uint32_t link;
        uint32_t info;
        uint64_t entsize;
} ElfWSection;

typedef struct
{
        uint32_t type;
        uint32_t flags;
        uint64_t align;

        /* Filled during layout */
        uint64_t offset;
        uint64_t vaddr;
        uint64_t paddr;
        uint64_t filesz;
        uint64_t memsz;

        /* Which sections are covered */
        size_t *section_indices;
        size_t section_count;
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

static inline void elfw_section_destroy(ElfWSection *s) { (void)s; } // TODO
static inline void elfw_segment_destroy(ElfWSegment *s) { (void)s; } // TODO

void elfw_destroy(ElfwCtx *ctx)
{
        if (ctx != NULL)
        {
                if(ctx->Head != NULL)
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
}

ElfResult elfw_create_header(ElfwCtx *ctx, const ElfwHeaderCreateInfo *info)
{
        if (ctx == NULL)
                return ELF_UNINIT;

        Elf64Header *header = malloc(sizeof(*header));
        if (header == NULL)
                return ELF_IO_ERROR; /* . //TODO: or ELF_NO_MEM if you add it */

        *header = (Elf64Header){
            .Info = {
                .Magic      = {0x7f, 'E', 'L', 'F'},
                .EI_Class   = info->class,
                .EI_Data    = info->endianness,
                .EI_Version = 1,
                .EI_OS_ABI  = info->os_abi,
                .EI_ABI_Version = info->abi_version,
                .Pad     = {0},
                .Type    = info->type,
                .Machine = info->machine,
                .Version = 1,
            },
            .Flags      = info->flags,
            .Entry      = info->entry,
            
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