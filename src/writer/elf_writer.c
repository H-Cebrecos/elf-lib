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
        EiClass Class;
        EiData Endianness;
        ElfType Type;
        ElfMachine Machine;
        ElfABI OS_ABI;
        uint8_t ABI_Version;

        uint64_t Entry;

        ElfWSection *Sections;
        size_t Section_count;
        size_t Section_cap;

        ElfWSegment *Segments;
        size_t Segment_count;
        size_t Segment_cap;
};

ElfwCtx *elfw_create(
    EiClass class,
    EiData data,
    ElfType type,
    ElfMachine machine,
    ElfABI os_abi,
    uint8_t abi_version,
    uint64_t entry)
{
        ElfwCtx *res;
        ElfwCtx *ctx = calloc(1, sizeof(*ctx));
        if (!ctx)
        {
                res = NULL;
        }
        else
        {

                ctx->Class = class;
                ctx->Endianness = data;

                /* Sensible defaults */
                ctx->Type = type;
                ctx->Machine = machine;
                ctx->OS_ABI = os_abi;
                ctx->ABI_Version = abi_version;
                ctx->Entry = entry;

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

// TODO: short doc, pass null if there are no tables.
static ElfResult elfw_write_header(const ElfwCtx *ctx, uint64_t phoff, uint64_t shoff)
{

        if (ctx == NULL)
                return ELF_UNINIT;

        Elf64Header header = {
            .Info = {
                .Magic = {0x7f, 'E', 'L', 'F'},
                .EI_Class = ctx->Class,
                .EI_Data = ctx->Endianness,
                .EI_Version = 1,
                .EI_OS_ABI = ctx->OS_ABI,
                .EI_ABI_Version = ctx->ABI_Version,
                .Pad = {0},
                .Type = ctx->Type,
                .Machine = ctx->Machine,
                .Version = 1,
            },
            .Entry = ctx->Entry,
            .ProHeadOff = phoff,
            .SecHeadOff = shoff,
        };

        // TODO: logic for the rest of the fields.
        // TODO: endianness aware writing
        // TODO: writer api.

        if (ctx->Class == CLASS_32)
        {
                //TODO!
        }
        else // 64 bit
        {
                
        }
}