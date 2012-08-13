#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "demo.h"

static void
stdio_read_func (png_structp png, png_bytep data, png_size_t size)
{
	FILE *file = png_get_io_ptr (png);
	while (size) {
		size_t ret;

		ret = fread (data, 1, size, file);
		size -= ret;
		data += ret;

		if (size && (feof (file) || ferror (file)))
			png_error (png, NULL);
	}
}

static void
png_simple_error_callback (png_structp png,
	                   png_const_charp error_msg)
{
#ifdef PNG_SETJMP_SUPPORTED
    longjmp (png_jmpbuf (png), 1);
#endif

    /* if we get here, then we have to choice but to abort ... */
}

static void
png_simple_warning_callback (png_structp png,
	                     png_const_charp error_msg)
{
}

static inline int
multiply_alpha (int alpha, int color)
{
	int temp = (alpha * color) + 0x80;
	return ((temp + (temp >> 8)) >> 8);
}

static void
premultiply_data (png_structp   png,
                  png_row_infop row_info,
                  png_bytep     data)
{
	unsigned int i;

	for (i = 0; i < row_info->rowbytes; i += 4) {
		uint8_t *base  = &data[i];
		uint8_t  alpha = base[3];
		uint32_t *p = (uint32_t *)base;

		if (alpha == 0) {
			*p = 0;
		} else {
			uint8_t  red   = base[0];
			uint8_t  green = base[1];
			uint8_t  blue  = base[2];

			if (alpha != 0xff) {
				red   = multiply_alpha (alpha, red);
				green = multiply_alpha (alpha, green);
				blue  = multiply_alpha (alpha, blue);
			}
			*p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
		}
	}
}

static void
convert_bytes_to_data (png_structp png, png_row_infop row_info, png_bytep data)
{
	unsigned int i;

	for (i = 0; i < row_info->rowbytes; i += 4) {
		uint8_t *base  = &data[i];
		uint8_t  red   = base[0];
		uint8_t  green = base[1];
		uint8_t  blue  = base[2];
		uint32_t *p = (uint32_t *)base;

		*p = (0xff << 24) | (red << 16) | (green << 8) | (blue << 0);
	}
}

static cairo_surface_t *
read_png (FILE *file, cairo_surface_t *other)
{
	cairo_surface_t *surface = NULL;
	png_byte **rows = NULL;
	png_struct *png;
	png_info *info;
	png_uint_32 width, height;
	uint8_t *data;
	int depth, color_type, interlace, stride;
	unsigned int i;
	cairo_format_t format;

	png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,
				      png_simple_error_callback,
				      png_simple_warning_callback);
	if (png == NULL)
		return NULL;

	info = png_create_info_struct (png);
	if (info == NULL)
		goto BAIL;

	png_set_read_fn (png, file, stdio_read_func);

#ifdef PNG_SETJMP_SUPPORTED
	if (setjmp (png_jmpbuf (png)))
		goto BAIL;
#endif

	png_read_info (png, info);

	png_get_IHDR (png, info,
		      &width, &height, &depth,
		      &color_type, &interlace, NULL, NULL);

	/* convert palette/gray image to rgb */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (png);

	/* expand gray bit depth if needed */
	if (color_type == PNG_COLOR_TYPE_GRAY) {
#if PNG_LIBPNG_VER >= 10209
		png_set_expand_gray_1_2_4_to_8 (png);
#else
		png_set_gray_1_2_4_to_8 (png);
#endif
	}

	/* transform transparency to alpha */
	if (png_get_valid (png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png);

	if (depth == 16)
		png_set_strip_16 (png);

	if (depth < 8)
		png_set_packing (png);

	/* convert grayscale to RGB */
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb (png);

	if (interlace != PNG_INTERLACE_NONE)
		png_set_interlace_handling (png);

	png_set_filler (png, 0xff, PNG_FILLER_AFTER);

	/* recheck header after setting EXPAND options */
	png_read_update_info (png, info);
	png_get_IHDR (png, info,
		      &width, &height, &depth,
		      &color_type, &interlace, NULL, NULL);
	if (depth != 8 ||
	    ! (color_type == PNG_COLOR_TYPE_RGB ||
	       color_type == PNG_COLOR_TYPE_RGB_ALPHA))
		goto BAIL;

	switch (color_type) {
	default:
		/* fall-through just in case ;-) */
	case PNG_COLOR_TYPE_RGB_ALPHA:
		format = CAIRO_FORMAT_ARGB32;
		png_set_read_user_transform_fn (png, premultiply_data);
		break;

	case PNG_COLOR_TYPE_RGB:
		format = CAIRO_FORMAT_RGB24;
		png_set_read_user_transform_fn (png, convert_bytes_to_data);
		break;
	}

	surface = cairo_surface_create_similar_image (other,
						      format, width, height);
	if (cairo_surface_status (surface))
		goto BAIL;

	rows = malloc (height * sizeof (char *));
	if (rows == NULL)
		goto BAIL;

	data = cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface);
	for (i = 0; i < height; i++)
		rows[i] = data + i*stride;

	png_read_image (png, rows);
	png_read_end (png, info);
	free (rows);

	cairo_surface_mark_dirty (surface);
	return surface;

BAIL:
	free (rows);
	if (png != NULL)
		png_destroy_read_struct (&png, &info, NULL);
	cairo_surface_destroy (surface);
	return NULL;
}

cairo_surface_t *
surface_create_from_png (cairo_surface_t *other, const char *filename)
{
	FILE *file;
	cairo_surface_t *surface;

	if (cairo_version () < CAIRO_VERSION_ENCODE (1, 12, 0))
		return cairo_image_surface_create_from_png (filename);

	file = fopen (filename, "rb");
	if (file == NULL)
		return NULL;

	surface = read_png (file, other);
	fclose (file);

	return surface;
}
