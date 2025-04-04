# SDF Mono Atlas Generator
This is a simple tool for generating ASCII SDF bitmaps that can be used for easy font rendering.

## Example
```
❯ sdf_mono_atlas -o my_atlas.png my_font.ttf
cell width = 20px
cell height = 27px
num cells = 95
baseline = 18px
```
![font atlas example](example.png "Generated font file")

## Requirements
- Operating System: Linux
- Architecture: x86_x64
- Shared Libraries: libm, libc

## Installation
- Arch: `yay -S sdf_mono_atlas`
- [Download prebuilt binary](https://github.com/jakubsmolaga/sdf_mono_atlas/releases)

## Building from source
Requirements:
- clang
- make
```
❯ git clone https://github.com/jakubsmolaga/sdf_mono_atlas.git
❯ cd sdf_mono_atlas
❯ make sdf_mono_atlas
```

## Help
```
❯ sdf_mono_atlas --help
Usage: sdf_mono_atlas [OPTION...] <TTF_FILE>
Simple utility for generating a monospaced SDF ASCII atlas

  -e, --on-edge=VALUE        Value 0-255 to use as a treshhold for determining
                             the glyph outline (default=180)
  -o, --output=PATH          Output location of the generated atlas
                             (default='sdf_atlas.png')
  -p, --padding=VALUE        Padding pixels for SDF (default=5)
  -q, --quiet                Output raw measurements, without descriptions.
                             Useful for scripts
  -s, --font-size=VALUE      Pixel height of the font, does not include padding
                             (default=22)
  -h, --help, --usage        Display this help and exit
```
