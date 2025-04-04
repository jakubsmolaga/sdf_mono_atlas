# SDF Mono Atlas Generator
This is a simple tool for generating ASCII SDF bitmaps that can be used for easy font rendering.
## Example
```
> sdf_mono_atlas -o my_atlas.png my_font.ttf

cell width = 20px
cell height = 27px
num cells = 95
baseline = 18px
```
![font atlas example](example.png "Generated font file")
## Help
```
❯ sdf_mono_atlas --help
Usage: sdf_mono_atlas [OPTION...] <TTF_FILE>
Simple utility for generating a monospaced SDF ASCII atlas

  -e, --on-edge=VALUE        Value 0-255 to use as a treshhold for determining
                             the glyph outline (default=180)
      --font-size=VALUE      Pixel height of the font, does not include padding
                             (default=22)
  -o, --output=PATH          Output location of the generated atlas
                             (default='sdf_atlas.png')
  -p, --padding=VALUE        Padding pixels for SDF (default=5)
  -q, -s, --quiet, --silent  Output raw measurements, without descriptions.
                             Useful for scripts
  -h, --help, --usage        Display this help and exit
```
