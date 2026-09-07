/* Render deterministic, real-world scenes for pixel-exact golden-image tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pte_fonts.h"

#ifdef PTE_GOLDEN_LVGL
#include "../../src/lvgl/lv_pte.c"
#else
#include "../../src/pte/pte.c"
#endif

#define WIDTH 1600
#define HEIGHT 1200

static unsigned char canvas[WIDTH * HEIGHT];
static int public_adapter;
static pte_base_font * fonts[4];

void hw_blendPixel(int x, int y, int alpha, int colour)
{
    if(x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT) {
        unsigned char * pixel = &canvas[y * WIDTH + x];
        *pixel = (unsigned char)(((int)*pixel * (256 - alpha) + colour * alpha) / 256);
    }
}

static int utf8(const unsigned char ** text)
{
    const unsigned char * p = *text;
    int code = *p++;
    if(code >= 0xc0) {
        int continuation = code < 0xe0 ? 1 : code < 0xf0 ? 2 : 3;
        code &= (1 << (6 - continuation)) - 1;
        while(continuation--) code = (code << 6) | (*p++ & 63);
    }
    *text = p;
    return code;
}

static void text_line(const pte_base_font * source, int size, int x, int y,
                      int rotation, const char * text)
{
#ifdef PTE_GOLDEN_LVGL
    if(public_adapter) {
        unsigned char bitmap[512 * 512];
        lv_font_t * font = lv_pte_create(source, size);
        const unsigned char * p = (const unsigned char *)text;
        while(*p) {
            int code = utf8(&p);
            const unsigned char * next = p;
            int next_code = *p ? utf8(&next) : 0;
            lv_font_glyph_dsc_t glyph = {0};
            if(!lv_font_get_glyph_dsc(font, &glyph, code, next_code)) continue;
            if(glyph.box_w > 512 || glyph.box_h > 512) abort();

            lv_draw_buf_t buffer = {0};
            buffer.header.stride = glyph.box_w;
            buffer.data_size = sizeof(bitmap);
            buffer.data = bitmap;
            lv_font_get_glyph_bitmap(&glyph, &buffer);

            for(int by = 0; by < glyph.box_h; ++by) {
                for(int bx = 0; bx < glyph.box_w; ++bx) {
                    int dx = x + glyph.ofs_x + bx;
                    int dy = y - glyph.box_h - glyph.ofs_y + by;
                    if(dx >= 0 && dx < WIDTH && dy >= 0 && dy < HEIGHT) {
                        int alpha = bitmap[by * glyph.box_w + bx];
                        unsigned char * pixel = &canvas[dy * WIDTH + dx];
                        *pixel = (unsigned char)((*pixel * (255 - alpha) + 32 * alpha) / 255);
                    }
                }
            }
            x += glyph.adv_w;
        }
        lv_pte_destroy(font);
        return;
    }

    pte_font_dsc_t descriptor = {0};
    descriptor.src = source;
    descriptor.size = size;
    pte_render_ctx_t context = {0};
    context.bitmap = canvas;
    context.stride = WIDTH;
    context.width = WIDTH;
    context.height = HEIGHT;
    const unsigned char * p = (const unsigned char *)text;
    const pte_glyph * previous = NULL;
    int pen = (x * source->m_size) / size;
    (void)rotation;
    while(*p) {
        int code = utf8(&p);
        const pte_glyph * glyph = find_glyph(source, code);
        if(!glyph) continue;
        if(previous) pen += find_kern(source, previous, code);
        draw_glyph(&descriptor, glyph, pen * size / source->m_size, y, &context);
        pen += glyph->xadvance;
        previous = glyph;
    }
#else
    pte_font font = pte_getFont(source, size);
    pte_drawText(&font, x, y, rotation, text, (size_t)-1, 32);
#endif
}

static const char * prose[] = {
    "The morning train arrived at 08:42, carrying commuters into the city.",
    "Please check your connection settings before starting the update.",
    "Kitchen temperature: 21.7 C | Humidity: 48% | Power today: 6.24 kWh",
    "A quick brown fox jumps over the lazy dog; FIVE BOXES cost $123.45.",
    "Meeting notes: review the prototype, confirm dates, and send feedback.",
    "Invoice #2026-0917: subtotal $149.95; tax $29.99; balance due $179.94.",
    "At the weekend we walked along the river and stopped for lunch.",
    "Network online (192.168.1.42). Download complete: 1,024 / 1,024 MB.",
    "Caf\xc3\xa9, cr\xc3\xa8me br\xc3\xbbl\xc3\xa9" "e, pi\xc3\xb1" "ata, fa\xc3\xa7" "ade, \xc3\x9c" "ber: d\xc3\xa9j\xc3\xa0 vu!",
    "AVATAR To Wa Yo: punctuation... [ready] {set} <go> / path\\file.txt",
    "Your order has shipped. Estimated arrival: Thursday, 17 September.",
    "Battery 87%   Signal -62 dBm   Uptime 12d 03h 27m   Status: healthy"
};

static const char * names[] = {
    "body_12", "body_18", "body_24", "body_32", "dashboard_24",
    "numbers_48", "mixed_styles", "rotations_18", "adapter_body_24"
};

static void scene(int scenario)
{
#ifdef PTE_GOLDEN_LVGL
    memset(canvas, scenario == 8 ? 255 : 0, sizeof(canvas));
#else
    memset(canvas, 255, sizeof(canvas));
#endif
    public_adapter = scenario == 8;
    if(public_adapter) scenario = 2;

    if(scenario < 4) {
        const int sizes[] = {12, 18, 24, 32};
        int size = sizes[scenario];
        for(int row = 0; row < 24; ++row) {
            text_line(fonts[(row / 6) % 4], size, 23 + row % 5,
                      50 + row * (size + 12), 0, prose[row % 12]);
        }
    }
    else if(scenario == 4) {
        for(int row = 0; row < 20; ++row) {
            for(int col = 0; col < 4; ++col) {
                char label[96];
                snprintf(label, sizeof(label), "Zone %02d: %2d.%d C / %d%%", row * 4 + col,
                         18 + row % 8, (row + col) % 10, 40 + row);
                text_line(fonts[(row + col) % 4], 24, 20 + col * 395,
                          50 + row * 52, 0, label);
            }
        }
    }
    else if(scenario == 5) {
        for(int row = 0; row < 12; ++row) {
            for(int col = 0; col < 4; ++col) {
                char label[64];
                snprintf(label, sizeof(label), "%02d:%02d:%02d", (row + col) % 24,
                         (row * 7 + col) % 60, (row * 13 + col * 9) % 60);
                text_line(fonts[col], 48, 20 + col * 395, 75 + row * 90, 0, label);
            }
        }
    }
    else if(scenario == 6) {
        const int sizes[] = {9, 13, 17, 23, 31, 47};
        for(int row = 0; row < 24; ++row) {
            text_line(fonts[row % 4], sizes[row % 6], 19 + row % 7, 55 + row * 46,
                      0, prose[row % 12]);
        }
    }
    else {
        for(int rotation = 0; rotation < 4; ++rotation) {
            for(int row = 0; row < 10; ++row) {
                int x = rotation == 1 ? 1550 - row * 30 : rotation == 2 ? 1500 : 40 + row * 30;
                int y = rotation == 3 ? 1150 : rotation == 2 ? 1100 - row * 30 : 40 + row * 30;
                text_line(fonts[row % 4], 18, x, y, rotation * 90, prose[row % 12]);
            }
        }
    }
}

static void write_scene(const char * directory, const char * name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.pgm", directory, name);
    FILE * file = fopen(path, "wb");
    if(!file) {
        perror(path);
        exit(1);
    }
    fprintf(file, "P5\n%d %d\n255\n", WIDTH, HEIGHT);
#ifdef PTE_GOLDEN_LVGL
    if(!public_adapter) {
        unsigned char row[WIDTH];
        for(int y = 0; y < HEIGHT; ++y) {
            for(int x = 0; x < WIDTH; ++x) {
                row[x] = (unsigned char)(255 - canvas[y * WIDTH + x] * 223 / 255);
            }
            fwrite(row, 1, WIDTH, file);
        }
    }
    else
#endif
    {
        fwrite(canvas, 1, sizeof(canvas), file);
    }
    fclose(file);
}

int main(int argc, char ** argv)
{
    if(argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    fonts[0] = get_Roboto_Regular();
    fonts[1] = get_Roboto_Bold();
    fonts[2] = get_Roboto_Italic();
    fonts[3] = get_Roboto_Bold_Italic();
#ifdef PTE_GOLDEN_LVGL
    lv_init();
#endif
    for(int scenario = 0; scenario < 9; ++scenario) {
#ifdef PTE_GOLDEN_LVGL
        if(scenario == 7) continue;
#else
        if(scenario == 8) continue;
#endif
        scene(scenario);
        write_scene(argv[1], names[scenario]);
    }
#ifdef PTE_GOLDEN_LVGL
    lv_deinit();
#endif
    return 0;
}
