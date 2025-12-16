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

#ifndef ELF_LIB
#define ELF_LIB

#include <stdint.h>
#include <stdbool.h>

#define ELF_CTX_SIZE 128u

/**
 * @brief Library context, holds internal information used by the library between function calls, initialized on Elf_init()
 */
typedef struct
{
        uint8_t _storage[ELF_CTX_SIZE];
} ElfCtx;

/****************
 *  ELF Header  *
 ****************/
        typedef enum EiClass
        {
                CLASS_NONE = 0,
                CLASS_32 = 1,
                CLASS_64 = 2,
        } EiClass;

        typedef enum EiData
        {
                DATA_NONE = 0,
                DATA_LSB = 1,
                DATA_MSB = 2,
        } EiData;

        typedef enum ElfType
        {
                TYPE_NONE = 0,
                TYPE_RELOC = 1,
                TYPE_EXEC = 2,
                TYPE_DYN = 3,
                TYPE_CORE = 4,
        } ElfType;

        typedef enum ElfMachine
        {
                MACHINE_NONE = 0,
                MACHINE_ARM = 40,
                MACHINE_RISCV = 243,
                MACHINE_AARCH64 = 183,
        } ElfMachine;

        typedef enum ElfABI
        {
                SYSTEM_V = 0 // This is the default value for most linkers
        } ElfABI;

        typedef struct
        {
                EiClass EI_Class;       // 32 or 64 bit architecture
                EiData  EI_Data;         // Endianness of the architecture
                uint8_t EI_Version;     // Always 1
                ElfABI  EI_OS_ABI;       // Target Platform's ABI
                uint8_t EI_ABI_Version; // Target ABI Version
                uint8_t Pad[7];
                ElfType Type;        // Type of ELF file
                ElfMachine Machine;  // Architecture
                uint32_t Version;    // Always 1
                uint64_t Entry;      // Entry point (virtual address)
                uint64_t ProHeadOff; // Offset of program header table in the file
                uint64_t SecHeadOff; // Offset of section header table in the file
                uint32_t Flags;
                uint16_t HeadSize;       // This header's size
                uint16_t PHEntrySize;    // Size of one entry in the program header table
                uint16_t PHEntryNum;     // Number of entries in the program header table
                uint16_t SHEntrySize;    // Size of one entry in the section header table
                uint16_t SHEntryNum;     // Number of entries in the section header table
                uint16_t SecNameStrIndx; // Index of the entry in the section table that points to the section names
        } ElfHeader;

/****************
 *   Sections   *
 ****************/        
        typedef enum ElfSectionType
        {
                SECTION_NULL = 0,
                SECTION_PROGBITS = 1,
                SECTION_SYMTAB  = 2,
                SECTION_STRTAB  = 3,
                SECTION_RELA    = 4,
                SECTION_HASH    = 5,
                SECTION_DYNAMIC = 6,
                SECTION_NOTE   = 7,
                SECTION_NOBITS = 8,
                SECTION_REL    = 9,
                SECTION_SHLIB  = 10,
                SECTION_DYNSYM = 11,
                SECTION_OTHER = 12,
        } ElfSectionType;

        typedef enum ElfSectionFlag
        {
                SEC_FLAG_WRITE = 0x01, // Contains writable data
                SEC_FLAG_ALLOC = 0x02, // Section is allocated in the memory image of the program
                SEC_FLAG_INSTR = 0x04  // Contains executable instructions
        } ElfSectionFlag;

        typedef struct
        {
                uint32_t NameIdx;    // Index into the section header string table section
                ElfSectionType Type; // Type of section
                uint64_t Flags;
                uint64_t Address; // If the section is in the memory img of a process this is the first address
                uint64_t Offset;  // Section is stored at <offset> from the begining of this file
                uint64_t Size;    // Size of the section
                uint32_t Link;    // Index in the Sec Header Table of an associated section
                uint32_t Info;
                uint64_t Alignment; // Alignment constraints of Address field
                uint64_t EntrySize; // If the section is a table of fixed-size entries this is the entry size
        } ElfSecHeader;

/****************
 *   Segments   *
 ****************/         
        typedef enum ElfSegmentType
        {
                SEGMENT_NULL = 0,
                SEGMENT_LOAD = 1,
                SEGMENT_DYNAMIC = 2,
                SEGMENT_INTERP  = 3,
                SEGMENT_NOTE  = 4,
                SEGMENT_SHLIB = 5,
                SEGMENT_PHDR  = 6,
        } ElfSegmentType;



        typedef struct
        {
                ElfSegmentType Type; // Type of segment
                uint32_t Flags;      
                uint64_t Offset;     // Section is stored at <offset> from the begining of this file
                uint64_t PhyAddress; // Physical address, only relevant on some systems
                uint64_t VirAddress; // Virtual address of this segment in memory
                uint64_t FileSize;   // Size of the segment in this file
                uint64_t MemSize;    // Size of the segment in the memory image
                uint64_t Alignment;  // Alignment constraints of Address fields
        } ElfProHeader;

/***************
 *   Symbols   *
 ***************/  
        typedef enum ElfSymbolType
        {
                SYM_NOTYPE  = 0,
                SYM_OBJECT  = 1, // Data
                SYM_FUNC    = 2, // Function entry point
                SYM_SECTION = 3,
                SYM_FILE    = 4  // Source file associated with the object file 
        } ElfSymbolType;

        typedef enum ElfSymbolBind
        {
                BIND_LOCAL  = 0,  // Not visible outside the object file
                BIND_GLOBAL = 1,  // Visible to all symbol files
                BIND_WEAK   = 2   // Global scope, overriden by a global symbol of same name
        } ElfSymbolBind;
        
        typedef struct
        {
                uint32_t NameIdx;      // Index into the related string table
                ElfSymbolType Type;    // Type of data represented by the symbol
                ElfSymbolBind Binding; // Binding atributes of the symbol
                uint16_t SecIdx;       // Section table index
                uint64_t Value;        // Value (Address) of the symbol
                uint64_t Size;         // Size of the object referenced by the symbol
        } ElfSymTabEntry;
        

#define SHN_UNDEF 0 // Undefined, missing or irrelevant section reference

typedef enum ElfResult
{
        ELF_OK = 0,
        ELF_UNINIT,
        ELF_BAD_MAGIC,
        ELF_BAD_VERSION,
        ELF_BAD_CLASS,
        ELF_BAD_ENDIANNESS,
        ELF_BAD_ARG,
        ELF_NOT_FOUND,
        ELF_BUFFER_OVERFLOW,
        ELF_IO_EOF,
        ELF_IO_ERROR
} ElfResult;




/**
 * @brief callback abstracks how de system may be reading the elf file, it can be implemented over a file system, a contiguous memory region
 * or dynamically loaded upon request. The user_ctx can be used by the user to store data between calls to the function.
 */
typedef ElfResult (*elf_read_callback)(
    void *user_ctx,  // user-provided context (file handle, pointer, etc.)
    uint64_t offset, // absolute offset in the "file"
    size_t size,     // number of bytes requested
    uint8_t *buffer     // destination buffer (already allocated by caller a.k.a the library)
);


/**
 * @param user_ctx Pointer to user defined structure that holds arbitrary data needed by the callback. NULL if unused.
 * @param callback Callback function to abtract the IO implemetation of the elf file.
 * @param cxt Pointer to Lib context allocated by the caller.
 * @return Error code.
 * @brief Reads the ELF header to extract the needed information for subsequent calls of the library.
 */
ElfResult elf_init(void *user_ctx, elf_read_callback callback, ElfCtx *cxt);

/**
 * @param ctx Lib context, initialized on Elf_init().
 * @param header (out) User allocated struct to be filled.
 * @return Error code
 */
ElfResult parse_header(const ElfCtx *ctx, ElfHeader *header);

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
 * @param ctx Lib context, initialized on Elf_init().
 * @param sec_idx  Index of the string table in the section header table
 * @param str_idx  Index of the string inside the string table section
 * @param buff (out) Character buffer to return the string.
 * @param len lenght of "buff".
 * @return Error code
 * @brief  Gets the string in the <str_idx> position from a string table section.
 */
ElfResult get_str_from_table(const ElfCtx *ctx, uint32_t sec_idx, uint32_t str_idx, uint8_t *buff, uint16_t len);


#endif // Include guard
