#include "common/elf_core.h"
#include "reader/elf_reader.h" 

static const char *class_to_str(EiClass c)
{
    switch (c)
    {
    case ELFCLASS32:
        return "ELF32";
    case ELFCLASS64:
        return "ELF64";
    default:
        return "none";
    }
}

static const char *data_to_str(EiData d)
{
    switch (d)
    {
    case ELFDATA2LSB:
        return "2's complement, little endian";
    case ELFDATA2MSB:
        return "2's complement, big endian";
    default:
        return "unknown";
    }
}

static const char *type_to_str(ElfType t)
{
    switch (t)
    {
    case ET_REL:
        return "REL (Relocatable file)";
    case ET_EXEC:
        return "EXEC (Executable file)";
    case ET_DYN:
        return "DYN (Shared object file)";
    case ET_CORE:
        return "CORE (Core file)";
    default:
        return "NONE";
    }
}

static const char *machine_to_str(ElfMachine m)
{
    switch (m)
    {
    //case :
    //    return "ARM";
    //case MACHINE_RISCV:
    //    return "RISC-V";
    //case MACHINE_AARCH64:
    //    return "AArch64";
    default:
        return "Unknown";
    }
}

static const char *abi_to_str(ElfABI m)
{
    switch (m)
    {
    case ELFOSABI_NONE:
        return "UNIX - SYSTEM V";
    default:
        return "Unknown";
    }
}

static const char *elferr_to_str(ElfResult e)
{
    switch (e)
    {
    case ELF_OK:
        return "OK";
    case ELF_BAD_MAGIC:
        return "Bad magic";
    case ELF_BAD_VERSION:
        return "Bad version";
    case ELF_BAD_CLASS:
        return "Bad class";
    case ELF_BAD_ENDIANNESS:
        return "Bad endianness";
    case ELF_IO_ERROR:
        return "I/O error";
    default:
        return "Unknown error";
    }
}

static const char *segment_type_to_str(ElfSegmentType t)
{
    switch (t)
    {
    case SEGMENT_NULL:
        return "NULL";
    case SEGMENT_LOAD:
        return "LOAD";
    case SEGMENT_DYNAMIC:
        return "DYNAMIC";
    case SEGMENT_INTERP:
        return "INTERP";
    case SEGMENT_NOTE:
        return "NOTE";
    case SEGMENT_SHLIB:
        return "SHLIB";
    case SEGMENT_PHDR:
        return "PHDR";
    default:
        return "UNKNOWN";
    }
}

static const char *sym_type_to_str(ElfSymbolType t)
{
    switch (t)
    {
    case STT_NOTYPE:  return "NOTYPE";
    case STT_OBJECT:  return "OBJECT";
    case STT_FUNC:    return "FUNC";
    case STT_SECTION: return "SECTION";
    case STT_FILE:    return "FILE";
    default:          return "UNK";
    }
}

static const char *sym_bind_to_str(ElfSymbolBind b)
{
    switch (b)
    {
    case STB_LOCAL:  return "LOCAL";
    case STB_GLOBAL: return "GLOBAL";
    case STB_WEAK:   return "WEAK";
    default:         return "UNK";
    }
}
