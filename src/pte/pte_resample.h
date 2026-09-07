/* Internal scanline resampler shared by the native and LVGL backends.
 * Copyright (c) 2015, Matt Pyne. SPDX-License-Identifier: BSD-3-Clause
 *
 * The caller supplies PTE_RESAMPLE_ALLOC and PTE_RESAMPLE_FREE. All scratch
 * storage belongs to the draw call; there is no global cache or mutable state.
 */
#ifndef PTE_RESAMPLE_H
#define PTE_RESAMPLE_H

#include "pte.h"

typedef struct {
    const unsigned char * ptr;
    unsigned char bit_mask;
    int col;
    int pixels_to_go;
    int switch_col;
} pte_rice_state;

typedef struct {
    int span;                  /* Includes the original transparent X padding. */
    int samples;               /* Number of compressed source pixels to consume. */
    unsigned int previous;     /* Unnormalised coverage of the last literal row. */
    unsigned int total;        /* Coverage accumulated over this Y group. */
} pte_resample_bucket;

typedef struct {
    pte_resample_bucket * buckets;
    size_t capacity;
} pte_resample_buffer;

typedef struct {
    const unsigned char * repeat_rows;
    int row;
    pte_rice_state rice;
    pte_resample_bucket * buckets;
    int count;
} pte_bitmap_decoder;

static int riceReadBit(pte_rice_state * state)
{
    int bit = (*state->ptr & state->bit_mask) != 0;
    state->bit_mask >>= 1;
    if(state->bit_mask == 0) {
        state->bit_mask = 0x80;
        ++state->ptr;
    }
    return bit;
}

static int riceReadRun(pte_rice_state * state)
{
    int quotient = 0;
    int remainder = 0;
    while(!riceReadBit(state)) ++quotient;
    for(int i = 0; i < 4; ++i) remainder = (remainder << 1) | riceReadBit(state);
    return quotient * 16 + remainder;
}

static void bitmapDecoderInit(pte_bitmap_decoder * decoder, const pte_base_font * font,
                              const pte_glyph * glyph)
{
    const unsigned char * data = font->m_data + glyph->ptr;
    decoder->row = 0;
    decoder->repeat_rows = data;
    decoder->rice.ptr = data + ((glyph->height + 7) / 8);
    decoder->rice.bit_mask = 0x80;
    decoder->rice.col = 0;
    decoder->rice.pixels_to_go = 0;
    decoder->rice.switch_col = 0;
    decoder->buckets = NULL;
    decoder->count = 0;
}

/* Precompute the EXACT boundaries of the old c -= ra; c += rb loop once
 * per glyph. A bucket always consumes at least one X step, even at ra > rb.
 * Do not replace span with samples: padding contributes to the denominator.
 */
static int pteResamplePrepare(pte_bitmap_decoder * decoder, pte_resample_buffer * buffer,
                             int src_width, int ra, int rb, int sub_offset_x)
{
    int x = -((rb * sub_offset_x) / ra) / rb;
    int distance = src_width - x;
    int step = ra < rb ? ra : rb;
    int count = distance > 0 ? (int)(((int64_t)distance - 1) * step / rb) + 1 : 1;
    int64_t c = rb;
    if((size_t)count > buffer->capacity) {
        if((size_t)count > SIZE_MAX / sizeof(*buffer->buckets)) return 0;
        pte_resample_bucket * replacement = (pte_resample_bucket *)
            PTE_RESAMPLE_ALLOC((size_t)count * sizeof(*replacement));
        if(!replacement) return 0;
        PTE_RESAMPLE_FREE(buffer->buckets);
        buffer->buckets = replacement;
        buffer->capacity = (size_t)count;
    }
    decoder->buckets = buffer->buckets;
    decoder->count = count;
    for(int p = 0; p < count; ++p) {
        int span = c > 0 ? (int)((c + ra - 1) / ra) : 1;
        int end = x + span;
        int first = x > 0 ? x : 0;
        int last = end < src_width ? end : src_width;
        decoder->buckets[p].span = span;
        decoder->buckets[p].samples = last > first ? last - first : 0;
        decoder->buckets[p].previous = 0;
        x = end;
        c += rb - (int64_t)span * ra;
    }
    return 1;
}

static int pteRepeatedRow(const pte_bitmap_decoder * decoder, int row)
{
    return (decoder->repeat_rows[row >> 3] & (0x80 >> (row & 7))) != 0;
}

/* Consume runs in chunks bounded by destination buckets. A white run skips
 * its source pixels; a black run adds its length instead of incrementing once
 * for every pixel. Runs may continue across literal source scanlines.
 */
static void pteResampleLiteral(pte_bitmap_decoder * decoder)
{
    pte_rice_state * state = &decoder->rice;
    for(int p = 0; p < decoder->count; ++p) {
        int remaining = decoder->buckets[p].samples;
        unsigned int coverage = 0;
        while(remaining > 0) {
            while(state->pixels_to_go == 0) {
                if(state->switch_col) state->col = !state->col;
                state->pixels_to_go = riceReadRun(state);
                state->switch_col = 1;
            }
            int take = state->pixels_to_go < remaining ? state->pixels_to_go : remaining;
            if(state->col) coverage += (unsigned int)take;
            state->pixels_to_go -= take;
            remaining -= take;
        }
        decoder->buckets[p].previous = coverage;
    }
}

/* Repeated rows reuse raw horizontal coverage, including repetitions spanning
 * multiple Y groups. Multiplicity is limited to this group, so every source
 * row retains its contribution and the caller's overspill remains unchanged.
 */
static void pteResampleRows(pte_bitmap_decoder * decoder, int lines)
{
    int remaining = lines;
    for(int p = 0; p < decoder->count; ++p) decoder->buckets[p].total = 0;
    while(remaining > 0) {
        if(!pteRepeatedRow(decoder, decoder->row)) pteResampleLiteral(decoder);
        int copies = 1;
        while(copies < remaining && pteRepeatedRow(decoder, decoder->row + copies)) ++copies;
        for(int p = 0; p < decoder->count; ++p)
            decoder->buckets[p].total += decoder->buckets[p].previous * (unsigned int)copies;
        decoder->row += copies;
        remaining -= copies;
    }
}

#endif
