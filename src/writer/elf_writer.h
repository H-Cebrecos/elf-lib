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

#ifndef ELFW_LIB
#define ELFW_LIB

#include "src/common/elf_core.h"

/**
 * @brief Library context, holds internal information used by the library between function calls, initialized on elfw_create()
 */
typedef struct ElfwCtx ElfwCtx;

typedef enum {
    ELFW_LAYOUT_FAST,
    ELFW_LAYOUT_COMPAT,
    ELFW_LAYOUT_PACKED,
    ELFW_LAYOUT_MINIMAL
} ElfwLayoutPolicy;

//TODO: think about this.
// Same callback shape as reader, but used for writing
// Returns ELF_OK on success
typedef ElfResult (*ElfIOCallback)(
        void *user,
        uint64_t offset,
        uint64_t size,
        const void *buffer);


/**
 * Create a new ELF writer context.
 *
 * Allocates and initializes an ELF writer context that will be used to
 * construct a valid ELF file in memory. The parameters provided here
 * define the fixed properties of the ELF header and cannot be changed
 * after creation.
 *
 * This function allocates heap memory; the returned context must be
 * released with elfw_destroy().
 *
 * @param class        ELF class (ELFCLASS32 or ELFCLASS64).
 * @param data         Data encoding / endianness (ELFDATA2LSB or ELFDATA2MSB).
 * @param type         ELF file type (e.g. ET_EXEC, ET_DYN, ET_REL).
 * @param machine      Target machine architecture (e.g. EM_X86_64).
 * @param os_abi       Target OS ABI (e.g. ELFOSABI_SYSV).
 * @param abi_version  ABI version, interpretation depends on OS ABI.
 * @param entry        Entry point virtual address (e_entry).
 *
 * @return Pointer to a newly created ElfwCtx on success, or NULL on
 *         allocation or parameter validation failure.
 */
ElfwCtx *elfw_create(void);

/**
 * Destroy an ELF writer context.
 *
 * Frees all resources owned by the context, including any sections,
 * segments, or auxiliary buffers that were created during ELF
 * construction. Passing NULL is allowed and has no effect.
 *
 * After this call, the context pointer must not be used again.
 *
 * @param ctx Pointer to an ElfwCtx previously returned by elfw_create().
 */
void     elfw_destroy(ElfwCtx *ctx);

typedef struct
{
        EiClass     class;
        EiData      endianness;
        ElfType     type;
        ElfMachine  machine;
        ElfABI      os_abi;
        uint8_t     abi_version;

        uint64_t    entry;
        uint32_t    flags;
} ElfwHeaderCreateInfo;

//TODO: this is good for now but it leaks to much info to the user, split into internal doc and API doc.
/**
 * @param ctx  Writer context, initialized with elfw_create().
 * @param info Header creation parameters describing the ELF file identity
 *             and layout (class, endianness, ABI, entry point, and table offsets).
 * @return Error code.
 *
 * @brief Creates and initializes the ELF file header.
 *
 * Allocates and initializes an ELF header according to the values provided in
 * @p info. The header is stored internally in the writer context and may be
 * further updated during layout (e.g. program/section header counts and
 * string table index).
 *
 * The program header table and section header table offsets may be zero to
 * indicate that the corresponding table is not present.
 *
 * This function does not write any data to the output; it only prepares the
 * in-memory representation of the ELF header.
 */
ElfResult elfw_create_header(ElfwCtx *ctx, const ElfwHeaderCreateInfo *info);



#endif // Include guard;