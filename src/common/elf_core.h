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

        typedef enum ElfType
        {
                ET_NONE         = 0, // No type
                ET_REL          = 1, // relocatable
                ET_EXEC         = 2, // executable
                ET_DYN          = 3, // shared object
                ET_CORE         = 4, // Core file
                ET_LOOS    = 0xfe00, // start of OS-specific range

                /* You may add your application's OS-specific types here */

                ET_HIOS    = 0xfeff, //   end of OS-specific range
                ET_LOPROC  = 0xff00, // start of Processor-specific range

                /* You may add your application's processor-specific types here */

                ET_HIPROC  = 0xffff, //   end of Processor-specific range
        } ElfType;

        typedef enum ElfVersion
        {
                EV_NONE    = 0, // Invalid
                EV_CURRENT = 1,
        } ElfVersion;

        typedef enum ElfMachine
        {
                EM_NONE = 0,
        } ElfMachine;

        typedef enum EiClass
        {
                ELFCLASSNONE = 0,
                ELFCLASS32   = 1,
                ELFCLASS64   = 2,
        } EiClass;

        typedef enum EiData
        {
                ELFDATANONE = 0,
                ELFDATA2LSB = 1,
                ELFDATA2MSB = 2,
        } EiData;

        typedef enum ElfABI
        {
                ELFOSABI_NONE = 0 // This is the default value for most linkers
        } ElfABI;

        /**
         * @brief Abtract representation of the ELF header, it does not represent the real layout, instead provides a uniform view into the data.
         */
        typedef struct
        {
                EiClass EI_Class;       // 32 or 64 bit architecture
                EiData  EI_Data;        // Endianness of the architecture
                uint8_t EI_Version;     // Always 1
                ElfABI  EI_OS_ABI;      // Target Platform's ABI
                uint8_t EI_ABI_Version; // Target ABI Version
                uint8_t Pad[7];
                ElfType Type;           // Type of ELF file
                ElfMachine Machine;     // Architecture
                uint32_t Version;       // Always 1
                uint64_t Entry;         // Entry point (virtual address)
                uint64_t ProHeadOff;    // Offset of program header table in the file
                uint64_t SecHeadOff;    // Offset of section header table in the file
                uint32_t Flags;
                uint16_t HeadSize;      // This header's size
                uint16_t PHEntrySize;   // Size of one entry in the program header table
                uint16_t PHEntryNum;    // Number of entries in the program header table
                uint16_t SHEntrySize;   // Size of one entry in the section header table
                uint16_t SHEntryNum;    // Number of entries in the section header table
                uint16_t SecStrIndx;    // Index of the entry in the section table that points to the section names
        } ElfHeader;

/****************
 *   Sections   *
 ****************/        
        typedef enum ElfSectionType
        {
                SHT_NULL          = 0,
                SHT_PROGBITS      = 1,
                SHT_SYMTAB        = 2,
                SHT_STRTAB        = 3,
                SHT_RELA          = 4,
                SHT_HASH          = 5,
                SHT_DYNAMIC       = 6,
                SHT_NOTE          = 7,
                SHT_NOBITS        = 8,
                SHT_REL           = 9,
                SHT_SHLIB         = 10,
                SHT_DYNSYM        = 11,
                SHT_INIT_ARRAY    = 14,
                SHT_FINI_ARRAY    = 15,
                SHT_PREINIT_ARRAY = 16,
                SHT_GROUP         = 17,
                SHT_SYMTAB_SHNDX  = 18,
                SHT_RELR          = 19,
                SHT_LOOS          = 0x60000000,

                /* You may add your application's OS-specific types here */

                SHT_HIOS          = 0x6fffffff,
                SHT_LOPROC        = 0x70000000,

                /* You may add your application's processor-specific types here */

                SHT_HIPROC        = 0x7fffffff,
                SHT_LOUSER        = 0x80000000,
                SHT_HIUSER        = 0xffffffff,
        } ElfSectionType;

        typedef enum ElfSectionFlag
        {
               SHF_WRITE            = 0x1,
               SHF_ALLOC            = 0x2,
               SHF_EXECINSTR        = 0x4,
               SHF_MERGE            = 0x10,
               SHF_STRINGS          = 0x20,
               SHF_INFO_LINK        = 0x40,
               SHF_LINK_ORDER       = 0x80,
               SHF_OS_NONCONFORMING = 0x100,
               SHF_GROUP            = 0x200,
               SHF_TLS              = 0x400,
               SHF_COMPRESSED       = 0x800,
               SHF_MASKOS           = 0x0ff00000,
               /* You may add your application's OS-specific flags here */
               SHF_MASKPROC         = 0xf0000000,
               /* You may add your application's processor-specific flags here */
        } ElfSectionFlag;

        typedef enum SecGrpFlags
        {
                GRP_COMDAT      = 0x1,
                GRP_MASKOS      = 0x0ff00000,
                /* You may add your application's OS-specific flags here */
                GRP_MASKPROC    = 0xf0000000,
                /* You may add your application's processor-specific flags here */
        }SecGrpFlags;
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
                STT_NOTYPE  = 0,
                STT_OBJECT  = 1, // Data
                STT_FUNC    = 2, // Function entry point
                STT_SECTION = 3,
                STT_FILE    = 4,  // Source file associated with the object file
                STT_COMMON  = 5,
                STT_TLS     = 6,
                STT_LOOS    = 10, 
                /* You may add your application's OS-specific flags here */
                STT_HIOS    = 12, 
                STT_LOPROC  = 13,
                /* You may add your application's processor-specific flags here */
                STT_HIPROC  = 15, 
        } ElfSymbolType;

        typedef enum ElfSymbolBind
        {
                STB_LOCAL  = 0,  // Not visible outside the object file
                STB_GLOBAL = 1,  // Visible to all symbol files
                STB_WEAK   = 2,  // Global scope, overriden by a global symbol of same name
                STB_LOOS   = 10,
                /* You may add your application's OS-specific flags here */
                STB_HIOS   = 12,
                STB_LOPROC = 13,
                /* You may add your application's processor-specific flags here */
                STB_HIPROC = 15,
        } ElfSymbolBind;

        //TODO: doc each of these, page 33
        typedef enum ElfSymbolVis
        {
                STV_DEFAULT   = 0,
                STV_INTERNAL  = 1,
                STV_HIDDEN    = 2,
                STV_PROTECTED = 3,
                STV_EXPORTED  = 4,
                STV_SINGLETON = 5,
                STV_ELIMINATE = 6,
        }ElfSymbolVis;
        
        typedef struct
        {
                uint32_t NameIdx;      // Index into the related string table
                ElfSymbolType Type;    // Type of data represented by the symbol
                ElfSymbolBind Binding; // Binding atributes of the symbol
                uint16_t SecIdx;       // Section table index //TODO: better expl
                uint64_t Value;        // Value (Address) of the symbol
                uint64_t Size;         // Size of the object referenced by the symbol
        } ElfSymTabEntry;
        
/**
 * @brief Return codes for ELF library functions.
 *
 * These values indicate the result of an operation, including success,
 * various types of invalid input or format errors, I/O errors, and
 * buffer issues. They are used consistently across the library API.
 */
typedef enum ElfResult
{
        ELF_OK = 0,
        ELF_UNINIT,
        ELF_BAD_MAGIC,
        ELF_BAD_VERSION,
        ELF_BAD_CLASS,
        ELF_BAD_ENDIANNESS,
        ELF_BAD_ARG,            // usually a NULL pointer
        ELF_BAD_INDX,           // index would overflow the selected table
        ELF_NOT_FOUND,
        ELF_BUFFER_OVERFLOW,
        ELF_IO_EOF,
        ELF_IO_ERROR,
        ELF_NO_MEM
} ElfResult;

/**
 * @brief callback abstracks how de system may be reading the elf file, it can be implemented over a file system, a contiguous memory region
 * or dynamically loaded upon request. The user_ctx can be used by the user to store data between calls to the function.
 */
typedef ElfResult (*elf_read_callback)(
    void *user_ctx,  // user-provided context (file handle, pointer, etc.)
    uint64_t offset, // absolute offset in the "file"
    uint64_t size,     // number of bytes requested
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

#endif // Include guard;
