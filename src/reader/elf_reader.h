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

#ifndef ELF_READ_INC
#define ELF_READ_INC

#include "common/elf_core.h"

 /**
 * @param ctx Lib context, initialized on Elf_init().
 * @param header (out) User allocated struct to be filled.
 * @return Error code
 */
ElfResult get_header(const ElfCtx *ctx, ElfHeader *header);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @return Number of section headers in the file.
 */
uint16_t get_section_count(const ElfCtx *ctx);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @return Number of program headers in the file.
 */
uint16_t get_program_header_count(const ElfCtx *ctx);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param idx Index of the section in the section header table.
 * @param sec_header (out) User allocated struct to be filled.
 * @return Error code
 */
ElfResult get_section_header(const ElfCtx *ctx, uint32_t idx, ElfSecHeader *sec_header);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param sec_header  Section header structure.
 * @param buff (out) Character buffer to return the string.
 * @param len lenght of "buff".
 * @return Error code
 */
ElfResult get_section_name(const ElfCtx *ctx, const ElfSecHeader *sec_header, uint8_t *buff, uint16_t len);

/**
 * @param ctx Lib context, initialized with Elf_init().
 * @param name Null-terminated section name to search for.
 * @param sec (out) User-allocated structure to receive the section header.
 * @return Error code.
 */
ElfResult get_section_by_name(const ElfCtx *ctx, const uint8_t *name, ElfSecHeader *sec);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param idx Index of the section in the section header table.
 * @param prog_header (out) User allocated struct to be filled.
 * @return Error code
 */
ElfResult get_program_header(const ElfCtx *ctx, uint32_t idx, ElfProHeader *prog_header);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param sym_tab Section header corresponding to the symbol table (SECTION_SYMTAB or SECTION_DYNSYM).
 * @return Number of symbols in the table.
 */
uint32_t get_symbol_count(const ElfCtx *ctx, const ElfSecHeader *sym_tab);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param sym_tab Section header corresponding to the symbol table (SECTION_SYMTAB or SECTION_DYNSYM).
 * @param idx Index of the symbol entry within the symbol table.
 * @param sym (out) User allocated struct to be filled.
 * @return Error code
 * @brief Reads a single symbol entry from a symbol table section and fills the provided structure.
 */
ElfResult get_symbol_entry(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint32_t idx, ElfSymTabEntry *sym);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param str_tab_idx  Index into the section header table of the string table related to this symbols table (Link field).
 * @param sym  Symbol entry in the symbol table for which the name will be obtained
 * @param buff (out) Character buffer to return the string.
 * @param len lenght of "buff".
 * @return Error code
 */
ElfResult get_symbol_name(const ElfCtx *ctx, uint32_t str_tab_idx, const ElfSymTabEntry *sym, uint8_t *buff, uint16_t len);

/**
 * @param ctx Lib context, initialized with Elf_init().
 * @param sym_tab Section header for the symbol table.
 * @param addr Symbol address to match exactly.
 * @param sym (out) User-allocated structure to receive the symbol entry.
 * @return Error code
 * @brief Returns the symbol whose value exactly matches `addr`.
 *
 * The symbol's Value field must be equal to `addr`. Symbol size and type are
 * not considered. Undefined symbols (SHN_UNDEF) are ignored.
 *
 * If multiple symbols match, the first matching symbol encountered in the
 * symbol table is returned.
 */
ElfResult get_symbol_by_addr_exact( const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint64_t addr, ElfSymTabEntry *sym);

/**
 * @param ctx Lib context, initialized with Elf_init().
 * @param sym_tab Section header for the symbol table.
 * @param addr Address to look up.
 * @param sym (out) User-allocated structure to receive the symbol entry.
 * @return Error code.
 * @brief Returns the first symbol whose address range contains `addr`.
 *
 * The lookup considers only symbols that represent addressable objects
 * (functions or data) as these are the only symbol kinds that meaningfully occupy address ranges.
 * Ignores zero-sized and undefined symbols, and matches symbols whose range satisfies:
 *     start <= addr < start + size
 *
 * This behavior is suitable for mapping instruction or data addresses back to
 * their defining function or object
 * 
 * For exact address matches or label-like symbols, use
 * `get_symbol_by_addr_exact()`.   
 */
ElfResult get_symbol_by_addr_range(const ElfCtx *ctx, const ElfSecHeader *sym_tab, uint64_t addr, ElfSymTabEntry *sym);

/**
 * @param ctx Lib context, initialized with Elf_init().
 * @param name Null-terminated symbol name to search for.
 * @param sym_tab Section header for the symbol table.
 * @param sym (out) User-allocated structure to receive the symbol entry.
 * @return Error code.
 */
ElfResult get_symbol_by_name(const ElfCtx *ctx, const uint8_t *name, const ElfSecHeader *sym_tab, ElfSymTabEntry *sym);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param sec_idx  Index of the string table in the section header table
 * @param str_idx  Index of the string inside the string table section
 * @param buff (out) Character buffer to return the string.
 * @param len lenght of "buff".
 * @return Error code
 * @brief  Gets the string in the <str_idx> position from a string table section.
 */
ElfResult get_str_from_table(const ElfCtx *ctx, uint32_t sec_idx, uint32_t str_idx, uint8_t *buff, uint16_t len);

#endif // include guard;