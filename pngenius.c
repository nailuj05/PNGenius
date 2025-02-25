#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define GRID 40
#define COLORA 128
#define COLORB 200

void read_png_file(const char *input_file, const char *output_file) {
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("error opening file");
        return;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "error creating read struct\n");
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "error creating info struct\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "error reading png\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    // printf("W: %d, H: %d, D: %d, C: %d\n", width, height, bit_depth, color_type);

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));

    png_read_image(png, row_pointers);

    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            png_bytep px = &(row[x * 4]); // RGBA

						float alpha = (float)px[3]/255.0;
						
						if((x % GRID < GRID * 0.5) ^ (y % GRID < GRID * 0.5)) {
							px[0] = px[0] * alpha + COLORA * (1 - alpha); 
							px[1] = px[1] * alpha + COLORA * (1 - alpha); 
							px[2] = px[2] * alpha + COLORA * (1 - alpha); 
						} else {
							px[0] = px[0] * alpha + COLORB * (1 - alpha); 
							px[1] = px[1] * alpha + COLORB * (1 - alpha); 
							px[2] = px[2] * alpha + COLORB * (1 - alpha); 
						}
							
						px[3] = 255;
        }
    }

		// save new file
    FILE *out_fp = fopen(output_file, "wb");
    if (!out_fp) {
        perror("error opening output file");
        return;
    }

    png_structp png_out = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_out) {
        fprintf(stderr, "error creating write struct\n");
        fclose(out_fp);
        return;
    }

    png_infop info_out = png_create_info_struct(png_out);
    if (!info_out) {
        fprintf(stderr, "error creating info struct\n");
        png_destroy_write_struct(&png_out, NULL);
        fclose(out_fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_out))) {
        fprintf(stderr, "error during writing\n");
        png_destroy_write_struct(&png_out, &info_out);
        fclose(out_fp);
        return;
    }

    png_init_io(png_out, out_fp);
    png_set_IHDR(
        png_out, info_out, width, height,
        8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_out, info_out);
    png_write_image(png_out, row_pointers);
    png_write_end(png_out, NULL);

    // Cleanup
    fclose(out_fp);
    png_destroy_write_struct(&png_out, &info_out);
    png_destroy_read_struct(&png, &info, NULL);

    for (int y = 0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    printf("fixed png saved as %s\n", output_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.png> <output.png>\n", argv[0]);
        return 1;
    }

    read_png_file(argv[1], argv[2]);
    return 0;
}
