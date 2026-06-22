/**
 * @file lv_pte.c
 *
 * Portable Type Engine font adapter
 *
 * The PTE rasterizer is derived from portable-type-engine:
 * Copyright (c) 2015, Matt Pyne
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_pte.h"

#include <limits.h>

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    const pte_base_font * src;
    int32_t size;
    uint32_t last_metrics_code;
    int32_t last_metrics_ofs_x;
    int32_t last_metrics_ofs_y;
    int32_t last_metrics_box_w;
    int32_t last_metrics_box_h;
} pte_font_dsc_t;

typedef struct {
    uint8_t * bitmap;
    uint32_t stride;
    int32_t width;
    int32_t height;
    bool measure;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
} pte_render_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool pte_get_glyph_dsc_cb(const lv_font_t * font, lv_font_glyph_dsc_t * dsc, uint32_t letter,
                                 uint32_t next);
static const void * pte_get_glyph_bitmap_cb(lv_font_glyph_dsc_t * dsc, lv_draw_buf_t * draw_buf);
static const pte_glyph * find_glyph(const pte_base_font * src, uint32_t code);
static const pte_kern_entry * find_kern(const pte_base_font * src, const pte_glyph * first,
                                        uint32_t second_code);
static void measure_glyph(pte_font_dsc_t * dsc, const pte_glyph * glyph);
static void draw_glyph(const pte_font_dsc_t * dsc, const pte_glyph * glyph, int32_t x, int32_t y,
                       pte_render_ctx_t * ctx);
#if LVGL_VERSION_MAJOR > 9 || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 5)
static lv_font_t * pte_font_create_cb(const lv_font_info_t * info, const void * src);
static void pte_font_delete_cb(lv_font_t * font);
#endif

/**********************
 *  GLOBAL VARIABLES
 **********************/

#if LVGL_VERSION_MAJOR > 9 || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 5)
const lv_font_class_t lv_pte_font_class = {
    .create_cb = pte_font_create_cb,
    .delete_cb = pte_font_delete_cb,
};
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_font_t * lv_pte_create(const pte_base_font * src, int32_t font_size)
{
    lv_font_t * font = lv_malloc_zeroed(sizeof(lv_font_t));
    if(font == NULL) return NULL;
    if(!lv_pte_init(font, src, font_size)) {
        lv_free(font);
        return NULL;
    }

    return font;
}

bool lv_pte_init(lv_font_t * font, const pte_base_font * src, int32_t font_size)
{
    LV_ASSERT_NULL(font);
    LV_ASSERT_NULL(src);

    if(font == NULL || src == NULL || src->m_data == NULL || src->m_gylphs == NULL ||
       src->m_number_glyphs == 0 || src->m_size <= 0 || font_size <= 0) {
        LV_LOG_ERROR("invalid PTE font source or size");
        return false;
    }

    pte_font_dsc_t * dsc = lv_malloc_zeroed(sizeof(pte_font_dsc_t));
    if(dsc == NULL) return false;

    lv_memzero(font, sizeof(*font));
    dsc->src = src;
    dsc->last_metrics_code = UINT32_MAX;
    font->dsc = dsc;
    font->get_glyph_dsc = pte_get_glyph_dsc_cb;
    font->get_glyph_bitmap = pte_get_glyph_bitmap_cb;
    font->subpx = LV_FONT_SUBPX_NONE;
    font->kerning = LV_FONT_KERNING_NORMAL;
    font->underline_position = -1;
    font->underline_thickness = 1;
    lv_pte_set_size(font, font_size);
    return true;
}

void lv_pte_set_size(lv_font_t * font, int32_t font_size)
{
    LV_ASSERT_NULL(font);
    if(font == NULL || font_size <= 0) {
        LV_LOG_ERROR("invalid PTE font size");
        return;
    }

    pte_font_dsc_t * dsc = (pte_font_dsc_t *)font->dsc;
    dsc->size = font_size;
    dsc->last_metrics_code = UINT32_MAX;
    font->line_height = (dsc->src->m_line_height * font_size) / dsc->src->m_size;
    font->base_line = font->line_height - (dsc->src->m_baseline * font_size) / dsc->src->m_size;
}

void lv_pte_destroy(lv_font_t * font)
{
    if(font == NULL) return;
    lv_pte_deinit(font);
    lv_free(font);
}

void lv_pte_deinit(lv_font_t * font)
{
    if(font == NULL) return;
    lv_free((void *)font->dsc);
    lv_memzero(font, sizeof(*font));
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t scale_value(const pte_font_dsc_t * dsc, int32_t value)
{
    return (value * dsc->size) / dsc->src->m_size;
}

#if LVGL_VERSION_MAJOR > 9 || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 5)
static lv_font_t * pte_font_create_cb(const lv_font_info_t * info, const void * src)
{
    lv_font_t * font = lv_pte_create(src, info->size);
    if(font != NULL) font->kerning = info->kerning;
    return font;
}

static void pte_font_delete_cb(lv_font_t * font)
{
    lv_pte_destroy(font);
}
#endif

static int32_t scale_box(const pte_font_dsc_t * dsc, int32_t value)
{
    if(value <= 0) return 0;
    return (value * dsc->size + dsc->src->m_size - 1) / dsc->src->m_size;
}

static const pte_glyph * find_glyph(const pte_base_font * src, uint32_t code)
{
    int32_t low = 0;
    int32_t high = (int32_t)src->m_number_glyphs - 1;
    while(low <= high) {
        int32_t mid = low + (high - low) / 2;
        if(src->m_gylphs[mid].code == (int)code) return &src->m_gylphs[mid];
        if(src->m_gylphs[mid].code < (int)code) low = mid + 1;
        else high = mid - 1;
    }
    return NULL;
}

static const pte_kern_entry * find_kern(const pte_base_font * src, const pte_glyph * first,
                                        uint32_t second_code)
{
    if(first == NULL || src->m_glyph_kern_rows == NULL || src->m_kern_rows == NULL ||
       src->m_kern_entries == NULL) return NULL;

    const pte_glyph * second = find_glyph(src, second_code);
    if(second == NULL) return NULL;
    uint16_t row_index = src->m_glyph_kern_rows[first - src->m_gylphs];
    if(row_index == PTE_NO_KERN_ROW) return NULL;

    const pte_kern_row * row = &src->m_kern_rows[row_index];
    uint32_t second_index = (uint32_t)(second - src->m_gylphs);
    uint32_t low = row->offset;
    uint32_t high = row->offset + row->count;
    while(low < high) {
        uint32_t mid = low + (high - low) / 2;
        const pte_kern_entry * entry = &src->m_kern_entries[mid];
        if(entry->second_glyph == second_index) return entry;
        if(entry->second_glyph < second_index) low = mid + 1;
        else high = mid;
    }
    return NULL;
}

static void blend_pixel(pte_render_ctx_t * ctx, int32_t x, int32_t y, int32_t alpha)
{
    if(alpha <= 0 || x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return;

    if(ctx->measure) {
        ctx->min_x = LV_MIN(ctx->min_x, x);
        ctx->min_y = LV_MIN(ctx->min_y, y);
        ctx->max_x = LV_MAX(ctx->max_x, x);
        ctx->max_y = LV_MAX(ctx->max_y, y);
        return;
    }

    uint8_t * pixel = ctx->bitmap + y * ctx->stride + x;
    *pixel = LV_MIN(alpha, 255);
}

static void blt_horz_cmprs_resize(const uint8_t ** ptr, bool * high_nibble, int32_t * col,
                                  int32_t * pixels_to_go, bool * switch_col,
                                  int32_t src_width, int32_t dst_x, int32_t dst_y, int32_t ra, int32_t rb,
                                  int32_t sub_offset_x, int32_t lines, int32_t overspill, pte_render_ctx_t * ctx)
{
    int32_t x_start = -((rb * sub_offset_x) / ra) / rb;
    size_t line_acc_size = (size_t)src_width + (size_t)(-x_start) + 1;
    uint32_t * line_acc = lv_malloc_zeroed(line_acc_size * sizeof(*line_acc));
    int32_t x = x_start;
    int32_t c = rb;
    int32_t p = 0;
    int32_t count = lines;
    int32_t div = 0;
    if(line_acc == NULL) return;

    while(count > 0) {
        if(x >= 0 && x < src_width) {
            while(*pixels_to_go == 0) {
                if(*switch_col) {
                    *col = !*col;
                    *switch_col = false;
                }
                int32_t run = *high_nibble ? (**ptr >> 4) : (**ptr & 0x0f);
                if(!*high_nibble) ++(*ptr);
                *high_nibble = !*high_nibble;
                *pixels_to_go = run;
                *switch_col = run < 15;
            }
            if(*col) ++line_acc[p];
            --(*pixels_to_go);
        }
        div += lines + overspill;
        ++x;
        c -= ra;

        if(c <= 0 || count == 0) {
            if(count <= 1) {
                if(line_acc[p] > 0 && div > 0) {
                    blend_pixel(ctx, dst_x, dst_y, (line_acc[p] << 8) / div);
                }
                dst_x++;
            }
            if(x >= src_width) {
                --count;
                p = 0;
                x = x_start;
                c = rb;
            }
            else {
                c += rb;
                ++p;
            }
            div = 0;
        }
    }

    lv_free(line_acc);
}

static void draw_glyph(const pte_font_dsc_t * dsc, const pte_glyph * glyph, int32_t x, int32_t y,
                       pte_render_ctx_t * ctx)
{
    const int32_t ra = dsc->size;
    const int32_t rb = dsc->src->m_size;
    int32_t acc = rb;
    bool high_nibble = true;
    int32_t col = 0;
    int32_t pixels_to_go = 0;
    bool switch_col = false;
    int32_t cy = 0;
    int32_t last_cy = 0;
    bool finished = false;
    const uint8_t * ptr = dsc->src->m_data + glyph->ptr;

    x = (x * rb) / ra;
    y = (y * rb) / ra;
    int32_t offset_x = ((x + glyph->xoffset) * ra) / rb;
    int32_t sub_offset_x = ((x + glyph->xoffset) * ra) % rb;
    int32_t offset_y = ((y - glyph->yoffset) * ra) / rb;
    int32_t sub_offset_y = rb - ((y - glyph->yoffset) * ra) % rb - 1;
    if(sub_offset_y) --offset_y;

    while(!finished) {
        acc -= ra;
        if(acc <= 0) {
            int32_t lines = cy - last_cy;
            int32_t overspill = 0;
            if(sub_offset_y > 0) {
                overspill = cy - (sub_offset_y * cy) / rb;
                lines -= overspill;
                cy -= overspill;
                sub_offset_y = 0;
            }
            else if(cy >= glyph->height) {
                overspill = cy - glyph->height;
                lines -= overspill;
                finished = true;
            }

            if(lines > 0) {
                blt_horz_cmprs_resize(&ptr, &high_nibble, &col, &pixels_to_go, &switch_col,
                                      glyph->width, offset_x, offset_y, ra, rb, sub_offset_x, lines,
                                      overspill, ctx);
            }
            offset_y++;
            last_cy = cy;
            acc += rb;
        }
        ++cy;
    }
}

static void measure_glyph(pte_font_dsc_t * dsc, const pte_glyph * glyph)
{
    if(dsc->last_metrics_code == glyph->code) return;

    const int32_t baseline = (dsc->src->m_baseline * dsc->size) / dsc->src->m_size;
    const int32_t line_height = (dsc->src->m_line_height * dsc->size) / dsc->src->m_size;
    const int32_t neg_x_offset = glyph->xoffset < 0 ? -glyph->xoffset : 0;
    const int32_t origin_x = scale_box(dsc, neg_x_offset + 2) + 4;
    const int32_t origin_y = baseline + 4;
    pte_render_ctx_t ctx = {
        .width = LV_MAX(origin_x + scale_box(dsc, glyph->xadvance + glyph->width + 4) + 4, 8),
        .height = LV_MAX(line_height * 2 + 8, 8),
        .measure = true,
    };
    ctx.min_x = ctx.width;
    ctx.min_y = ctx.height;
    ctx.max_x = -1;
    ctx.max_y = -1;
    draw_glyph(dsc, glyph, origin_x, origin_y, &ctx);

    dsc->last_metrics_code = glyph->code;
    if(ctx.max_x < ctx.min_x || ctx.max_y < ctx.min_y) {
        dsc->last_metrics_ofs_x = 0;
        dsc->last_metrics_ofs_y = 0;
        dsc->last_metrics_box_w = 0;
        dsc->last_metrics_box_h = 0;
        return;
    }

    dsc->last_metrics_ofs_x = ctx.min_x - origin_x;
    dsc->last_metrics_ofs_y = ctx.min_y - origin_y - 1;
    dsc->last_metrics_box_w = ctx.max_x - ctx.min_x + 1;
    dsc->last_metrics_box_h = ctx.max_y - ctx.min_y + 2;
}

static bool pte_get_glyph_dsc_cb(const lv_font_t * font, lv_font_glyph_dsc_t * out, uint32_t letter,
                                 uint32_t next)
{
    pte_font_dsc_t * dsc = (pte_font_dsc_t *)font->dsc;
    const pte_glyph * glyph = find_glyph(dsc->src, letter);
    if(glyph == NULL) return false;

    measure_glyph(dsc, glyph);
    int32_t advance = scale_value(dsc, glyph->xadvance);
    const pte_kern_entry * kern = font->kerning == LV_FONT_KERNING_NORMAL ?
                                  find_kern(dsc->src, glyph, next) : NULL;
    if(kern != NULL) advance += scale_value(dsc, kern->amount);

    out->adv_w = (uint16_t)LV_MAX(advance, 0);
    out->ofs_x = dsc->last_metrics_ofs_x;
    out->ofs_y = -dsc->last_metrics_ofs_y - dsc->last_metrics_box_h;
    out->box_w = dsc->last_metrics_box_w;
    out->box_h = dsc->last_metrics_box_h;
    out->is_placeholder = 0;
    out->format = LV_FONT_GLYPH_FORMAT_A8;
    out->gid.index = letter;
    return true;
}

static const void * pte_get_glyph_bitmap_cb(lv_font_glyph_dsc_t * glyph_dsc, lv_draw_buf_t * draw_buf)
{
    pte_font_dsc_t * dsc = (pte_font_dsc_t *)glyph_dsc->resolved_font->dsc;
    const pte_glyph * glyph = find_glyph(dsc->src, glyph_dsc->gid.index);
    if(glyph == NULL || draw_buf == NULL) return NULL;

    measure_glyph(dsc, glyph);
    lv_memzero(draw_buf->data, draw_buf->header.stride * glyph_dsc->box_h);
    pte_render_ctx_t ctx = {
        .bitmap = draw_buf->data,
        .stride = draw_buf->header.stride,
        .width = glyph_dsc->box_w,
        .height = glyph_dsc->box_h,
    };
    draw_glyph(dsc, glyph, -dsc->last_metrics_ofs_x, -dsc->last_metrics_ofs_y, &ctx);
    return draw_buf;
}
