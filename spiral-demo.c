#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "demo.h"

static struct source{
	cairo_surface_t *surface;
	int width, height;
} *sources;
static int num_sources;

static void
fps_draw (cairo_t *cr, const char *name,
	  const struct timeval *last,
	  const struct timeval *now)
{
#define N_FILTER 25
    static double filter[25];
    static int filter_pos;
    cairo_text_extents_t extents;
    char buf[80];
    double fps, avg;
    int n, max;

    fps = now->tv_sec - last->tv_sec;
    fps += (now->tv_usec - last->tv_usec) / 1000000.;

    max = N_FILTER;
    avg = fps;
    if (filter_pos < max)
	max = filter_pos;
    for (n = 0; n < max; n++)
	avg += filter[n];
    avg /= max + 1;
    filter[filter_pos++ % N_FILTER] = fps;
    if (filter_pos < 5)
	    return;

    snprintf (buf, sizeof (buf), "%s: %.1f fps", name, 1. / avg);
    cairo_set_font_size (cr, 18);
    cairo_text_extents (cr, buf, &extents);

    cairo_rectangle (cr, 4-1, 4-1, extents.width+2, extents.height+2);
    cairo_set_source_rgba (cr, .0, .0, .0, .85);
    cairo_fill (cr);

    cairo_move_to (cr, 4 - extents.x_bearing, 4 - extents.y_bearing);
    cairo_set_source_rgb (cr, .95, .95, .95);
    cairo_show_text (cr, buf);
}

static cairo_surface_t *
_cairo_image_surface_create_from_pixbuf(const GdkPixbuf *pixbuf)
{
	gint width = gdk_pixbuf_get_width (pixbuf);
	gint height = gdk_pixbuf_get_height (pixbuf);
	guchar *gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
	int gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	int cairo_stride;
	guchar *cairo_pixels;
	cairo_format_t format;
	cairo_surface_t *surface;
	int j;

	if (n_channels == 3)
		format = CAIRO_FORMAT_RGB24;
	else
		format = CAIRO_FORMAT_ARGB32;

	surface = cairo_image_surface_create(format, width, height);
	if (cairo_surface_status(surface))
		return surface;

	cairo_stride = cairo_image_surface_get_stride (surface);
	cairo_pixels = cairo_image_surface_get_data(surface);

	for (j = height; j; j--) {
		guchar *p = gdk_pixels;
		guchar *q = cairo_pixels;

		if (n_channels == 3) {
			guchar *end = p + 3 * width;

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				q[0] = p[2];
				q[1] = p[1];
				q[2] = p[0];
#else
				q[1] = p[0];
				q[2] = p[1];
				q[3] = p[2];
#endif
				p += 3;
				q += 4;
			}
		} else {
			guchar *end = p + 4 * width;
			guint t1,t2,t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

			while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
				MULT(q[0], p[2], p[3], t1);
				MULT(q[1], p[1], p[3], t2);
				MULT(q[2], p[0], p[3], t3);
				q[3] = p[3];
#else
				q[0] = p[3];
				MULT(q[1], p[0], p[3], t1);
				MULT(q[2], p[1], p[3], t2);
				MULT(q[3], p[2], p[3], t3);
#endif

				p += 4;
				q += 4;
			}
#undef MULT
		}

		gdk_pixels += gdk_rowstride;
		cairo_pixels += cairo_stride;
	}

	return surface;
}

static void load_sources(const char *path, cairo_surface_t *target)
{
	struct dirent *de;
	DIR *dir;
	int count = 0;

	dir = opendir(path);
	if (dir == NULL)
		return;

	while ((de = readdir(dir))) {
		GdkPixbuf *pb;
		gchar *filename;
		cairo_surface_t *surface, *image;
		cairo_status_t status;
		cairo_t *cr;
		struct stat st;

		if (de->d_name[0] == '.')
			continue;

		filename = g_build_filename(path, de->d_name, NULL);

		if (stat(filename, &st)) {
			g_free(filename);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			load_sources(filename, target);
			g_free(filename);
			continue;
		}

		pb = gdk_pixbuf_new_from_file(filename, NULL);
		g_free(filename);

		if (pb == NULL)
			continue;

		image = _cairo_image_surface_create_from_pixbuf(pb);
		g_object_unref(pb);

		surface = cairo_surface_create_similar(target,
						       cairo_surface_get_content(image),
						       cairo_image_surface_get_width(image),
						       cairo_image_surface_get_height(image));

		cr = cairo_create(surface);
		cairo_set_source_surface(cr, image, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		status = cairo_status(cr);
		cairo_destroy(cr);

		if (status == CAIRO_STATUS_SUCCESS) {
			struct source *s;

			s = realloc(sources,
				    (num_sources+1)*sizeof(struct source));
			if (s) {
				s[num_sources].surface =
					cairo_surface_reference(surface);
				s[num_sources].width =
					cairo_image_surface_get_width(image);
				s[num_sources].height =
					cairo_image_surface_get_height(image);
				num_sources++;
				count++;
				sources = s;
			}
		}

		cairo_surface_destroy(image);
		cairo_surface_destroy(surface);
	}
	closedir(dir);

	if (count)
		printf("Loaded %d images from %s\n", count, path);
}

int main(int argc, char **argv)
{
	struct device *device;
	const char *path = "/usr/share/backgrounds";
	float sincos_lut[360];
	struct timeval start, last_tty, last_fps, now;
	int theta, frames, n;
	int show_path = 1;
	int show_outline = 1;
	int show_images = 1;
	int show_fps = 1;

	device = device_open(argc, argv);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--images") == 0) {
			path = argv[n+1];
			n++;
	    } else if (strcmp (argv[n], "--hide-path") == 0) {
		show_path = 0;
	    } else if (strcmp (argv[n], "--hide-outline") == 0) {
		show_outline = 0;
	    } else if (strcmp (argv[n], "--hide-images") == 0) {
		show_images = 0;
	    } else if (strcmp (argv[n], "--hide-fps") == 0) {
		show_fps = 0;
	    }
	}

	g_type_init();

	for (theta = 0; theta < 360; theta+= 2) {
		sincos_lut[theta+0] = sin(theta/180. * M_PI);
		sincos_lut[theta+1] = cos(theta/180. * M_PI);
	}

	load_sources(path, device->get_framebuffer(device)->surface);
	printf("Loaded %d images in total from %s\n", num_sources, path);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	frames = 0;
	do {
		struct framebuffer *fb;
		cairo_t *cr;
		double delta;
		int width = device->width;
		int height = device->height;
		int rotation;

		fb = device->get_framebuffer (device);
		cr = cairo_create (fb->surface);

		gettimeofday(&now, 0);
		delta = now.tv_sec - start.tv_sec;
		delta += (now.tv_usec - start.tv_usec)*1e-6;

		rotation = floor(50*delta);

		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		if (show_path) {
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_set_line_width(cr, 5);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

			cairo_move_to(cr, width/2, height/2);
			for (theta = 0; theta < 2000; theta += 5) {
				double dx = theta*sincos_lut[(((rotation+theta)%360)&~1)+0]/2;
				double dy = theta*sincos_lut[(((rotation+theta)%360)&~1)+1]/2;
				cairo_line_to(cr, width/2+dx, height/2+dy);
			}
			cairo_stroke(cr);
		}

		if (num_sources) {
			int step = 50;
			int image = num_sources;
			for (theta = 0; theta < 2000; theta += step) {
				double dx = theta*sincos_lut[(((rotation+theta)%360)&~1)+0]/2;
				double dy = theta*sincos_lut[(((rotation+theta)%360)&~1)+1]/2;
				double spin = 500*delta/(step+theta);

				struct source *source = &sources[image++%num_sources];
				int w = 2*ceil(step/M_SQRT2/5*4 * (1+theta/360.));
				int h = 2*ceil(step/M_SQRT2/5*3 * (1+theta/360.));

				cairo_save(cr);
				if (show_images) {
					cairo_translate(cr, width/2+dx, height/2+dy);
					cairo_rotate(cr, M_PI/2+spin+(rotation+theta)/180.*M_PI);
					cairo_scale(cr,
						    w/(double)source->width,
						    h/(double)source->height);
					cairo_set_source_surface(cr, source->surface,
								 -source->width/2,
								 -source->height/2);
					cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_NONE);
					cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
					cairo_identity_matrix(cr);
					cairo_paint(cr);
				}

				if (show_outline) {
					cairo_set_source_rgb(cr, 1, 1, 1);
					cairo_set_line_width(cr, 2);

					cairo_translate(cr, width/2+dx, height/2+dy);
					cairo_rotate(cr, M_PI/2+spin+(rotation+theta)/180.*M_PI);
					cairo_rectangle(cr, -w/2, -h/2, w, h);

					cairo_identity_matrix(cr);
					cairo_stroke(cr);
				}
				cairo_restore(cr);

				step += 5;
			}
		}

		if (show_fps) {
			if (last_fps.tv_sec)
				fps_draw(cr, device->name, &last_fps, &now);
			last_fps = now;
		}

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		frames++;
		delta = now.tv_sec - last_tty.tv_sec;
		delta += (now.tv_usec - last_tty.tv_usec)*1e-6;
		if (delta >  5) {
			printf("%.2f fps\n", frames/delta);
			last_tty = now;
			frames = 0;
		}
	} while (1);

	return 0;
}
