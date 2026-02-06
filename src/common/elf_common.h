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

//? NOTE: This is an internal file to host common structures to various elf related files, it should not be included into your project.

#ifndef ELF_COMM_LIB
#define ELF_COMM_LIB

/** Detect host endianness at runtime */
static inline EiData host_endianness(void) {
        uint16_t x = 1;
        return (*(uint8_t *)&x) ? ELFDATA2LSB : ELFDATA2MSB;
}

/** Swap helpers */
static inline uint16_t swap16(uint16_t v) {
        return (v >> 8) | (v << 8);
}
    
static inline uint32_t swap32(uint32_t v) {
        return ((v >> 24) & 0x000000FFUL) |
               ((v >> 8)  & 0x0000FF00UL) |
               ((v << 8)  & 0x00FF0000UL) |
               ((v << 24) & 0xFF000000UL);
}
    
static inline uint64_t swap64(uint64_t v) {
        return ((v >> 56) & 0x00000000000000FFULL) |
               ((v >> 40) & 0x000000000000FF00ULL) |
               ((v >> 24) & 0x0000000000FF0000ULL) |
               ((v >> 8)  & 0x00000000FF000000ULL) |
               ((v << 8)  & 0x000000FF00000000ULL) |
               ((v << 24) & 0x0000FF0000000000ULL) |
               ((v << 40) & 0x00FF000000000000ULL) |
               ((v << 56) & 0xFF00000000000000ULL);
}


#endif