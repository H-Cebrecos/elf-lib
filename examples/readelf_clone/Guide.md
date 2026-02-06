
# Understanding ELF Files by Re-Implementing `readelf`

This tutorial explains the structure of ELF files and demonstrates how to inspect them using this library.
The goal is to give a **clear mental model** of ELF rather than to hide details behind abstractions.

The intended reader is a programmer with basic C knowledge and limited familiarity with executable formats.

---

## What `readelf` Does

`readelf` is a diagnostic tool that reads ELF files and prints their internal structure.
It does **not** execute code and does **not** load the file into memory as a process.

Instead, it answers questions such as:

* What type of ELF file is this?
* What architecture was it built for?
* What sections does it contain?
* What symbols are defined?

Because it only parses metadata, `readelf` is an ideal reference when learning ELF.

---

## High-Level ELF File Layout

An ELF file is composed of a small number of well-defined tables.

```
File offset
0x0000
+--------------------+
| ELF Header         |  Fixed size, always first
+--------------------+
| Program Header Tbl |  Describes runtime memory layout
+--------------------+
| Section Data       |  Code, data, strings, symbols
|  (.text, .data…)   |
+--------------------+
| Section Header Tbl |  Describes file sections
+--------------------+
```

Key idea:

> **ELF separates “how the file is organized” from “how it is loaded”.**

This explains why there are two different header tables.

---

## Program Headers vs Section Headers

### Program Headers (Segments)

Program headers describe how the operating system maps the file into memory.

They are used by:

* the kernel loader
* the dynamic linker
* bootloaders

Each program header describes a **segment**:

```
Segment (PT_LOAD)
┌───────────────────────────┐
│ .text                     │
│ .rodata                   │
│ .eh_frame                 │
└───────────────────────────┘
```

A segment may contain multiple sections.
Segments are concerned with **memory**, not symbols or names.

---

### Section Headers (Sections)

Section headers describe the logical structure of the file.

They are used by:

* linkers
* debuggers
* inspection tools (`readelf`, `objdump`)

Typical sections:

```
.text     → executable code
.data     → initialized data
.bss      → zero-initialized data
.symtab   → symbols
.strtab   → strings
```

Sections are **not** loaded directly.
They exist to describe and group data inside the file.

---

## Relationship Between ELF Header and Tables

The ELF header acts as a directory.

```
ELF Header
   |
   |-- e_phoff ──> Program Header Table
   |                [0] PT_LOAD
   |                [1] PT_DYNAMIC
   |
   |-- e_shoff ──> Section Header Table
                    [0] SHT_NULL
                    [1] .text
                    [2] .data
                    [3] .symtab
                    [4] .strtab
```

This maps directly to the API:

* `get_program_header()`
* `get_section_header()`

---

## Why Section Names Are Indirect

Section headers do **not** store their names directly.

Instead:

* `sh_name` is an index
* the index points into a **string table**
* the string table is itself a section

Diagram:

```
Section Header (.text)
┌───────────────┐
│ sh_name = 12  │──┐
│ sh_type       │  │
│ sh_offset     │  │
└───────────────┘  │
                   ▼
.strtab section
┌────────────────────────┐
│ 0x00: ""               │
│ 0x01: ".symtab"        │
│ 0x09: ".strtab"        │
│ 0x12: ".text"          │  ← index 12
└────────────────────────┘
```

This design reduces duplication and allows names to be shared.

Because of this, section names are retrieved explicitly:

```c
get_section_name(ctx, &sh, buffer, size);
```

---

## Library Model

This library mirrors the ELF structure directly.

Design rules:

1. **No I/O inside the library**
   All data access happens through a user-provided callback.

2. **No hidden iteration**
   All traversal is explicit.

3. **Counts are queried explicitly**
   Helper functions exist to make iteration safe and simple.

The API favors clarity over automation.

---

## Required Setup

To inspect an ELF file you need:

* A read callback
* A caller-allocated `ElfCtx`
* An initialized ELF context

No global state is used.

---

## Step 1 — Implement a Read Callback

The callback must read `size` bytes at `offset`.

```c
ElfResult read_cb(void *user_ctx,
                  uint64_t offset,
                  size_t size,
                  void *buffer);
```

Example using `stdio`:

```c
static ElfResult file_read_cb(void *user_ctx,
                              uint64_t offset,
                              size_t size,
                              void *buffer)
{
    FILE *f = user_ctx;

    if (fseeko(f, offset, SEEK_SET) != 0)
        return ELF_IO_ERROR;

    if (fread(buffer, 1, size, f) != size)
        return feof(f) ? ELF_IO_EOF : ELF_IO_ERROR;

    return ELF_OK;
}
```

---

## Step 2 — Initialize the ELF Context

Initialization validates the ELF header and caches key values.

```c
ElfCtx ctx;
ElfResult res = elf_init(file, file_read_cb, &ctx);
```

After this call:

* class (32/64) is known
* endianness is known
* header sizes are validated
* special index cases are resolved

---

## Step 3 — Read the ELF Header (`readelf -h`)

```c
ElfHeader hdr;
get_header(&ctx, &hdr);
```

Relevant fields:

| Field        | Meaning              |
| ------------ | -------------------- |
| `EI_Class`   | 32-bit or 64-bit     |
| `EI_Data`    | Endianness           |
| `Type`       | REL / EXEC / DYN     |
| `Machine`    | Target architecture  |
| `Entry`      | Entry point address  |
| `PHEntryNum` | Program header count |
| `SHEntryNum` | Section header count |

---

## Step 4 — Iterate Over Section Headers (`readelf -S`)

The library provides a count helper:

```c
uint16_t count = get_section_count(&ctx);
```

Iteration becomes straightforward:

```c
for (uint16_t i = 0; i < count; i++)
{
    ElfSecHeader sh;
    get_section_header(&ctx, i, &sh);
}
```

Section names are retrieved explicitly:

```c
uint8_t name[256];
get_section_name(&ctx, &sh, name, sizeof(name));
```

---

## Step 5 — Iterate Over Program Headers (`readelf -l`)

```c
uint32_t ph_count = get_program_header_count(&ctx);

for (uint32_t i = 0; i < ph_count; i++)
{
    ElfProHeader ph;
    get_program_header(&ctx, i, &ph);
}
```

Program headers describe **runtime memory layout**, not file organization.

---

## Step 6 — Locate Symbol Tables

Symbol tables are sections of type:

* `SHT_SYMTAB`
* `SHT_DYNSYM`

```c
if (sh.Type == SHT_SYMTAB || sh.Type == SHT_DYNSYM)
{
    ...
}
```

Each symbol table references a string table via `sh.Link`.

---

## Step 7 — Iterate Over Symbols (`readelf -s`)

Symbol count is derived from the section itself:

```c
uint64_t sym_count = get_symbol_count(&ctx, &sh);
```

Reading symbols:

```c
for (uint64_t i = 0; i < sym_count; i++)
{
    ElfSymTabEntry sym;
    get_symbol_entry(&ctx, &sh, i, &sym);
}
```

Symbol names are retrieved separately:

```c
get_symbol_name(&ctx, sh.Link, &sym, name, sizeof(name));
```

---

## Error Handling

All functions return `ElfResult`.

The library does not terminate execution or print errors.
The caller decides whether an error is fatal or can be ignored.

This allows both strict validation tools and tolerant inspection tools.

---

## Recommended Learning Order

1. Print the ELF header
2. List section headers
3. List program headers
4. List symbol tables

Each step builds on the previous one and matches a `readelf` option.

---

## Mapping to `readelf`

| `readelf` | Library API                                                   |
| --------- | ------------------------------------------------------------- |
| `-h`      | `elf_init`, `get_header`                                      |
| `-S`      | `get_section_count`, `get_section_header`, `get_section_name` |
| `-l`      | `get_program_header_count`, `get_program_header`              |
| `-s`      | `get_symbol_count`, `get_symbol_entry`, `get_symbol_name`     |

---
