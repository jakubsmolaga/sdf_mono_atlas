sdf_mono_atlas: sdf_mono_atlas.c stb_truetype.h stb_image_write.h
	@clang -std=c99 -Wall -Wextra -lm -O2 -flto -o sdf_mono_atlas sdf_mono_atlas.c

compress: sdf_mono_atlas
	@upx --best --ultra-brute sdf_mono_atlas
