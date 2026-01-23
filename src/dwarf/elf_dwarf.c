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

//? NOTE: keep area of effect of switching elf_core low. this may be useful for other stuff.

#include <stddef.h>
#include "elf_dwarf.h"
#include "dwarf_consts.h"

#define CTX(ctx) ((InternalDwarfCtx *)(ctx))

//TODO: this may change to heap allocated copies of the section data.
typedef struct
{
        const ElfCtx *elf;
        ElfSecHeader debug_info;
        ElfSecHeader debug_abbrev;
        ElfSecHeader debug_str;
        uint8_t initialized;
} InternalDwarfCtx;

_Static_assert(sizeof(InternalDwarfCtx) <= DWARF_CTX_SIZE, "DWARF_CTX_SIZE too small");

/***************
 *    Abbrev   *
 ***************/ 
 // section 7.5.3 of spec v5
typedef struct {
    DwrfAttrName name;
    DwrfForm form;
    int64_t implicit_const;
} DwrfAttrSpec;

typedef struct {
        uint32_t code; //TODO: determine size
        uint16_t tag;
        bool has_children;
        //TODO: list of attrspecs. (num + pointer?)
        //parsing ends on some conditions like 0 name or somethin, look it up.
} DwrfAbbr;



DwrfResult dwarf_init(const ElfCtx *elf, DwrfCtx *ctx)
{
        ElfResult err;

        if ((elf == NULL)||(ctx == NULL))
                return DWRF_BAD_ARG;
        
        CTX(ctx)->elf = elf;
        CTX(ctx)->initialized = false;

        /**  TODO: only cache does that exist and set the rest to NULL,
         *   possible sections are:
         *      .debug_abbrev   - abbreviations used in .debug_info
         *      .debug_aranges  - map between addr and compilation
         *      .debug_frame    - call frame info
         *      .debug_info
         *      .debug_line     - line numbers program
         *      .debug_loclists - locations (replaces loc)
         *      .debug_macinfo  X macros (replaced with .debug_macro)
         *      .debug_macro    - macros 
         *      .debug_pubnames X LUT for global objects anf functions
         *      .debug_pubtypes X LUT for global types
         *      .debug_names    - replaces pubtypes and pubnames
         *      .debug_rnglits  - addr ranges (replaces ranges)
         *      .debug_str      - string table for .debug_info
         *      .debug_types    X type description (merged into .debug_info)
         * 
         *   It would be nice to know which of those are mandatory
         *   Also look up if version 5 introduces more.
         *       
         */      
        err  = get_section_by_name(elf, ".debug_info", &CTX(ctx)->debug_info);
        err += get_section_by_name(elf, ".debug_abbrev", &CTX(ctx)->debug_abbrev);
        err += get_section_by_name(elf, ".debug_str", &CTX(ctx)->debug_str);
        //TODO: maybe more...
        

        if(err)
                return DWRF_SEC_MISSING;

        CTX(ctx)->initialized = true;
        return DWRF_OK;
}

void dwarf_destroy(DwrfCtx *ctx)
{
 //TODO:
}

/** Helper expression to reduce repetition */
static inline DwrfResult validate_ctx(const DwrfCtx *ctx)
{
        if ((ctx == NULL) || !(CTX(ctx)->initialized))
                return DWRF_UNINIT;
        
        return DWRF_OK;
}

/**
 * ULEB128 encodes an unsigned integer using a variable number of bytes.
 *
 * The value is split into 7-bit groups, starting from the least significant
 * bits. Each group is stored in one byte. The most significant bit (bit 7)
 * of each byte is used as a continuation flag: it is set to 1 if another
 * byte follows, and set to 0 in the final (most significant) byte.
 *
 * Bytes are emitted in little-endian order, meaning the least significant
 * 7-bit group is stored first.
 * 
 * @param uleb128 Pointer to the start of the LEB128 encoded value.
 * @param val (out) Value of the ULEB128 represented as a u64.
 * @param len (out) Number of bytes consumed.
 * @return Error code
 */
static DwrfResult decode_ULEB128(const uint8_t* uleb128, uint64_t *val, uint8_t *len)
{
        uint8_t byte;
        uint8_t shift = 0;
        uint8_t count = 0;

        if((uleb128 == NULL) || (val == NULL) || (len == NULL))
                return DWRF_BAD_ARG;

        *val = 0;
        do
        {
                byte = *uleb128++;
                uint64_t temp = (uint64_t)(byte & 0x7FU);
                
                count++;

                /* if the shifted value overflows */
                if ((shift > 63) || ((temp << shift) >> shift != temp))
                        return DWRF_DECODE_ERR;

                *val |= temp << shift;
                shift += 7;
        } while (byte & 0x80);
        
        *len = count;
        return DWRF_OK;
}

/**
 * SLEB128 encodes a signed integer using a variable number of bytes.
 *
 * The value is split into 7-bit groups, starting from the least significant
 * bits. Each group is stored in one byte. The most significant bit (bit 7)
 * of each byte is used as a continuation flag: it is set to 1 if another
 * byte follows, and set to 0 in the final byte.
 *
 * The sign of the result is determined from bit 6 of the final byte and
 * sign-extended if required.
 *
 * Bytes are emitted in little-endian order, meaning the least significant
 * 7-bit group is stored first.
 *
 * @param sleb128 Pointer to the start of the LEB128 encoded value.
 * @param val (out) Value of the SLEB128 represented as an i64.
 * @param len (out) Number of bytes consumed.
 * @return Error code
 */
static DwrfResult decode_SLEB128(const uint8_t* sleb128, int64_t *val, uint8_t *len){
        uint8_t byte;
        uint8_t shift = 0;
        uint8_t count = 0;

        if((sleb128 == NULL) || (val == NULL) || (len == NULL))
                return DWRF_BAD_ARG;

        *val = 0;
        do
        {
                byte = *sleb128++;
                int64_t temp = (int64_t)(byte & 0x7F);

                count++;
                
                /* if the shifted value overflows */
                if ((shift > 63) || ((temp << shift) >> shift != temp))
                        return DWRF_DECODE_ERR;

                *val |= temp << shift;
                shift += 7;
        } while (byte & 0x80);

        /* sign extend if sign bit of last byte is set */
        if ((shift < 64) && (byte & 0x40))
                *val |=  (int64_t)((~0) << shift);
        
        *len = count;
        return DWRF_OK;
}
