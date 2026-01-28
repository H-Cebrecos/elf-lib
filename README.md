# Elf-lib

**Elf-lib** is a C library for reading and writing ELF files, designed with usability, portability, and modularity in mind.  

## Overview

Elf-lib provides an intuitive and straightforward API for working with ELF files. It is not intended to support every possible ELF file in existence, but it covers the vast majority of use cases, making it an excellent choice for homemade projects such as:

- Linkers  
- Assemblers and compilers  
- Bootloaders  

By handling much of the tedious boilerplate logic associated with reading and writing ELF files, Elf-lib allows you to focus on the core functionality of your project rather than low-level file manipulation.  

There are also plans to add support for ELF-adjacent formats such as DWARF for debug information.  

## Design

The library is organized into self-contained modules that you can include or exclude based on your needs. Apart from a few shared headers, each module can be built and integrated independently, making it easy to adapt the library to your project.  

### Modules

- **Reader Module**:  
  - Designed to be bootloader-compatible.  
  - No standard library dependencies; only uses stack memory.  
  - Can run on minimal C runtimes.  

- **Writer Module & Others**:  
  - Require basic memory allocation and optionally file output.  
  - Designed to be easily replaceable if you want to use custom allocators or I/O mechanisms.  

This modularity allows Elf-lib to be used in both highly constrained environments and more typical development scenarios.  

## Goals

- Simplify reading and writing ELF files without compromising flexibility.  
- Provide a usable and understandable API, even for "homemade" or experimental projects.  
- Support modular inclusion to minimize dependencies.  
- Enable compatibility with low-level environments such as bootloaders or custom OS projects.  
---

Elf-lib is aimed at developers who want practical, lightweight ELF support without the overhead of large, production-grade libraries.
