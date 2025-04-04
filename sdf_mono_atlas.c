#include <argp.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_truetype.h"

/* -------------------------------------------------------------------------- */
/*                                   types                                    */
/* -------------------------------------------------------------------------- */

typedef uint8_t  u8;
typedef int32_t  i32;
typedef float    f32;

struct Bitmap {
        u8 *data;
        i32 w, h;
        i32 xoff, yoff; // convienience, only used for glyphs
};
typedef struct Bitmap Bitmap;

/* -------------------------------------------------------------------------- */
/*                                 parameters                                 */
/* -------------------------------------------------------------------------- */

f32   pixel_height = 22.0;
f32   padding      = 5.0;
f32   onedge_value = 180.0;
char *output_path  = "sdf_atlas.png";
u8   *ttf_file     = 0;
bool  quiet        = false;
f32   pixel_dist_scale; // calculated as `onedge_value / padding`

/* -------------------------------------------------------------------------- */
/*                                  helpers                                   */
/* -------------------------------------------------------------------------- */

const i32 intmax = 0x7FFFFFFF;
const i32 intmin = 0x80000000;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static Bitmap
create_bitmap(i32 w, i32 h)
{
        return (Bitmap){.w = w, .h = h, .data = malloc(w * h)};
}

static void
bitmap_copy(Bitmap dst, Bitmap src, i32 x, i32 y)
{
        for (i32 dy = 0; dy < src.h; dy++) {
                for (i32 dx = 0; dx < src.w; dx++) {
                        u8 pixel = src.data[(dy * src.w) + dx];
                        dst.data[((y + dy) * dst.w) + x + dx] = pixel;
                }
        }
}

static u8 *
load_file(const char *filename)
{
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
                perror("fopen");
                return NULL;
        }
        i32 err = fseek(file, 0, SEEK_END);
        if (err != 0) {
                perror("fseek");
                return NULL;
        }
        i32 size = ftell(file);
        if (size < 0) {
                perror("ftell");
                return NULL;
        }
        err = fseek(file, 0, SEEK_SET);
        if (err != 0) {
                perror("fseek");
                return NULL;
        }
        u8 *buf = malloc(size);
        if (buf == NULL) {
                perror("malloc");
                return NULL;
        }
        size_t ok = fread(buf, size, 1, file);
        if (!ok) {
                perror("fread");
                return NULL;
        }
        return buf;
}

/* -------------------------------------------------------------------------- */
/*                               cli arguments                                */
/* -------------------------------------------------------------------------- */

static struct argp_option options[] = {
    {"help", 'h', 0, 0, "Display this help and exit"},
    {"usage", 0, 0, OPTION_ALIAS, 0},
    {"quiet", 'q', 0, 0,
     "Output raw measurements, without descriptions. Useful for scripts"},
    {"silent", 's', 0, OPTION_ALIAS, 0},
    {"font-size", 's', "VALUE", 0,
     "Pixel height of the font, does not include padding (default=22)"},
    {"padding", 'p', "VALUE", 0, "Padding pixels for SDF (default=5)"},
    {"on-edge", 'e', "VALUE", 0,
     "Value 0-255 to use as a treshhold for determining the glyph outline "
     "(default=180)"},
    {"output", 'o', "PATH", 0,
     "Output location of the generated atlas (default='sdf_atlas.png')"},
    {0},
};

static error_t
arg_parser(int key, char *arg, struct argp_state *state)
{
        switch (key) {
        case 'h':
                argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
                break;
        case 's':
                pixel_height = atof(arg);
                if (pixel_height <= 0) {
                        argp_error(state, "Invalid font size");
                }
                break;
        case 'p':
                padding = atof(arg);
                if (padding < 0) {
                        argp_error(state, "Invalid padding value");
                }
                break;
        case 'e':
                onedge_value = atof(arg);
                if (onedge_value < 0 | onedge_value > 255) {
                        argp_error(state, "Invalid on-edge value");
                }
                break;
        case 'o':
                output_path = arg;
                break;
        case 'q':
                quiet = true;
                break;
        case ARGP_KEY_ARG:
                if (state->arg_num >= 1) {
                        argp_error(state, "Too many positional arguments");
                }
                ttf_file = load_file(arg);
                if (ttf_file == NULL) {
                        argp_error(state, "Failed to load font file");
                }
                break;
        case ARGP_KEY_END:
                if (state->arg_num != 1) {
                        argp_error(state, "Please specify an input file");
                }
                break;
        default:
                return ARGP_ERR_UNKNOWN;
        }
        return 0;
}

static struct argp argp_config = {
    .options     = options,
    .parser      = arg_parser,
    .doc         = "Simple utility for generating a monospaced SDF ASCII atlas",
    .args_doc    = "<TTF_FILE>",
    .children    = 0,
    .help_filter = 0,
    .argp_domain = 0,
};

/* -------------------------------------------------------------------------- */
/*                                    main                                    */
/* -------------------------------------------------------------------------- */

int
main(int argc, char **argv)
{
        // handle cli arguments
        argp_parse(&argp_config, argc, argv, ARGP_NO_HELP, 0, 0);
        pixel_dist_scale = onedge_value / padding;

        // load the font
        stbtt_fontinfo font;
        stbtt_InitFont(&font, ttf_file, 0);

        // get scale factor for given font size
        f32 scale = stbtt_ScaleForPixelHeight(&font, pixel_height);

        // generate sdf bitmaps for all ascii characters
        // (except space because it gives weird metrics for some fonts)
        i32    x0 = intmax, y0 = intmax, x1 = intmin, y1 = intmin;
        Bitmap bitmaps[127];
        for (i32 c = 33; c < 127; c++) {
                bitmaps[c].data = stbtt_GetCodepointSDF(
                    &font, scale, c, padding, onedge_value, pixel_dist_scale,
                    &bitmaps[c].w, &bitmaps[c].h, &bitmaps[c].xoff,
                    &bitmaps[c].yoff);
                x0 = min(x0, bitmaps[c].xoff);
                y0 = min(y0, bitmaps[c].yoff);
                x1 = max(x1, bitmaps[c].xoff + bitmaps[c].w);
                y1 = max(y1, bitmaps[c].yoff + bitmaps[c].h);
        }

        // calculate atlas dimensions
        i32 cell_w    = x1 - x0;
        i32 cell_h    = y1 - y0;
        i32 baseline  = -y0;
        i32 num_cells = 127 - 32;

        // generate the atlas
        Bitmap atlas = create_bitmap(cell_w * num_cells, cell_h);
        for (i32 c = 33; c < 127; c++) {
                Bitmap b        = bitmaps[c];
                i32    x        = b.xoff + (i32)padding;
                i32    y        = b.yoff + baseline;
                i32    cell_idx = (c - 32);
                i32    cell_pos = cell_idx * cell_w;
                bitmap_copy(atlas, b, cell_pos + x, y);
        }

        // print atlas dimensions
        if (quiet) {
                printf("%d\n%d\n%d\n%d\n", cell_w, cell_h, num_cells, baseline);
        } else {
                printf("cell width = %dpx\n", cell_w);
                printf("cell height = %dpx\n", cell_h);
                printf("num cells = %d\n", num_cells);
                printf("baseline = %dpx\n", baseline);
        }

        // write the output png
        int ok =
            stbi_write_png(output_path, atlas.w, atlas.h, 1, atlas.data, 0);
        if (!ok) {
                perror("failed to write output file");
                exit(1);
        }

        return 0;
}
