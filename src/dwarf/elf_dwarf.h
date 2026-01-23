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

#ifndef DWARF_LIB
#define DWARF_LIB
 
#include "common/elf_core.h"
 
/** NOTE: This library supports DWARF version 5 based on ELF files */

#define DWARF_CTX_SIZE 256u

/**
 * @brief Opaque DWARF library context.
 * Internal storage used by the DWARF parser. Must be initialized by dwarf_init().
 */
typedef struct
{
        uint8_t _storage[DWARF_CTX_SIZE];
} DwrfCtx;


/**
 * @brief Return codes for DWARF library functions.
 *
 * These values indicate the result of an operation, including success,
 * various types of invalid input or format errors, I/O errors, and
 * buffer issues. They are used consistently across the library API.
 */
typedef enum DwrfResult{
        DWRF_OK = 0,
        DWRF_UNINIT,
        DWRF_BAD_ARG,
        DWRF_SEC_MISSING,
        DWRF_DECODE_ERR,
}DwrfResult;

/**
 * @brief Initialize the DWARF context from an ELF context.
 * @param elf Initialized ELF context (elf_init must be called first)
 * @param ctx Pointer to user-allocated DWARF context
 * @return ELF_OK or error code
 */
DwrfResult dwarf_init(const ElfCtx *elf, DwrfCtx *ctx);

void dwarf_destroy(DwrfCtx *ctx);

#endif //include guard;