#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "demo.h"

int device_get_size(int argc, char **argv, int *width, int *height)
{
	int w, h, n;

	for (n = 1; n < argc; n++) {
		if (strncmp (argv[n], "--size=", 7) == 0) {
			if (sscanf (argv[n]+7, "%dx%d", &w, &h) == 2) {
				*width = w;
				*height = h;
				return 1;
			}
		} else if (strcmp (argv[n], "--size") == 0) {
			if (n + 1 < argc &&
			    sscanf (argv[++n], "%dx%d", &w, &h) == 2) {
				*width = w;
				*height = h;
				return 1;
			}
		}
	}

	return 0;
}

enum split device_get_split(int argc, char **argv)
{
	enum split split = SPLIT_NONE;
	int n;

	for (n = 1; n < argc; n++) {
		if (strncmp (argv[n], "--split=", 8) == 0) {
			if (strcmp(argv[n]+8, "left") == 0) {
				split = SPLIT_LEFT;
			} else if (strcmp(argv[n]+8, "right") == 0) {
				split = SPLIT_RIGHT;
			} else if (strcmp(argv[n]+8, "bottom") == 0) {
				split = SPLIT_BOTTOM;
			} else if (strcmp(argv[n]+8, "top") == 0) {
				split = SPLIT_TOP;
			} else if (strcmp(argv[n]+8, "top-left") == 0) {
				split = SPLIT_TOP_LEFT;
			} else if (strcmp(argv[n]+8, "top-right") == 0) {
				split = SPLIT_TOP_RIGHT;
			} else if (strcmp(argv[n]+8, "bottom-left") == 0) {
				split = SPLIT_BOTTOM_LEFT;
			} else if (strcmp(argv[n]+8, "bottom-right") == 0) {
				split = SPLIT_BOTTOM_RIGHT;
			} else {
				split = SPLIT_1_16 + atoi(argv[n]+8);
			}
		}
	}

	return split;
}

const char *device_show_version(int argc, char **argv)
{
	int n;

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--show-version") == 0)
			return cairo_version_string();
	}

	return NULL;
}

int device_get_benchmark(int argc, char **argv)
{
	int n, count;

	count = -1;
	for (n = 1; n < argc; n++) {
		if (strncmp (argv[n], "--benchmark=", 12) == 0) {
			return atoi(argv[n]+12);
		} else if (strcmp (argv[n], "--benchmark") == 0) {
			if (n + 1 < argc)
				return atoi(argv[++n]);
			else
				return 0;
		}
	}

	return count;
}

static cairo_antialias_t str_to_antialias(const char *str)
{
	if (strcmp (str, "none") == 0)
		return CAIRO_ANTIALIAS_NONE;
#if CAIRO_VERSION > CAIRO_VERSION_ENCODE(1,11,3)
	else if (strcmp (str, "fast") == 0)
		return CAIRO_ANTIALIAS_FAST;
	else if (strcmp (str, "good") == 0)
		return CAIRO_ANTIALIAS_GOOD;
	else if (strcmp (str, "best") == 0)
		return CAIRO_ANTIALIAS_BEST;
#endif
	else
		return CAIRO_ANTIALIAS_DEFAULT;
}

const char *device_antialias_to_string(cairo_antialias_t antialias)
{
	switch (antialias) {
	case CAIRO_ANTIALIAS_NONE: return "none";
#if CAIRO_VERSION > CAIRO_VERSION_ENCODE(1,11,3)
	case CAIRO_ANTIALIAS_FAST: return "fast";
	case CAIRO_ANTIALIAS_BEST: return "best";
#endif
	case CAIRO_ANTIALIAS_GRAY: return "gray";
	case CAIRO_ANTIALIAS_SUBPIXEL: return "subpixel";
	default: return "default";
	}
}

cairo_antialias_t device_get_antialias(int argc, char **argv)
{
	int n;

	for (n = 1; n < argc; n++) {
		if (strncmp (argv[n], "--antialias=", 12) == 0) {
			return str_to_antialias(argv[n]+12);
		} else if (strcmp (argv[n], "--antialias") == 0) {
			if (n + 1 < argc)
				return str_to_antialias(argv[++n]);
			else
				return CAIRO_ANTIALIAS_DEFAULT;
		}
	}

	return CAIRO_ANTIALIAS_DEFAULT;
}

struct device *device_open(int argc, char **argv)
{
    struct device *device = 0;
    enum {
	AUTO,
	XLIB,
	XIMAGE,
	XSHM,
	SKIA,
	XCB,
	GLX,
	COGL,
	DRM,
    } backend = AUTO;
    int n;

    for (n = 1; n < argc; n++) {
	if (strcmp (argv[n], "--xlib") == 0) {
	    backend = XLIB;
	} else if (strcmp (argv[n], "--xcb") == 0) {
	    backend = XCB;
	} else if (strcmp (argv[n], "--ximage") == 0) {
	    backend = XIMAGE;
	} else if (strcmp (argv[n], "--xshm") == 0) {
	    backend = XSHM;
	} else if (strcmp (argv[n], "--drm") == 0) {
	    backend = DRM;
	} else if (strcmp (argv[n], "--glx") == 0) {
	    backend = GLX;
	} else if (strcmp (argv[n], "--cogl") == 0) {
	    backend = COGL;
	} else if (strcmp (argv[n], "--skia") == 0) {
	    backend = SKIA;
	}
    }

    if (backend == AUTO) {
	if (device == 0 && HAVE_DRM)
	    device = drm_open (argc, argv);
	if (device == 0 && HAVE_XLIB)
	    device = xlib_open (argc, argv);
	if (device == 0 && HAVE_XCB)
	    device = xcb_open (argc, argv);
	if (device == 0 && HAVE_GLX)
	    device = glx_open (argc, argv);
	if (device == 0 && HAVE_COGL)
	    device = cogl_open (argc, argv);
	if (device == 0 && HAVE_XIMAGE)
	    device = ximage_open (argc, argv);
	if (device == 0 && HAVE_XSHM)
	    device = xshm_open (argc, argv);
	if (device == 0 && HAVE_SKIA)
	    device = skia_open (argc, argv);
    } else switch (backend) {
    case AUTO:
    case XLIB:
	device = xlib_open (argc, argv);
	break;
    case XCB:
	device = xcb_open (argc, argv);
	break;
    case XIMAGE:
	device = ximage_open (argc, argv);
	break;
    case XSHM:
	device = xshm_open (argc, argv);
	break;
    case SKIA:
	device = skia_open (argc, argv);
	break;
    case GLX:
	device = glx_open (argc, argv);
	break;
    case COGL:
	device = cogl_open (argc, argv);
	break;
    case DRM:
#if HAVE_DRM
	device = drm_open (argc, argv);
#endif
	break;
    }

    if (device == 0) {
	fprintf(stderr, "Failed to open a drawing device\n");
	exit(1);
    }

    printf("Using backend \"%s\"\n", device->name);
    return device;
}

#if HAVE_GDK_PIXBUF
cairo_surface_t *
surface_create_from_pixbuf(cairo_surface_t *other, const GdkPixbuf *pixbuf)
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

	if (other == NULL || cairo_version () < CAIRO_VERSION_ENCODE (1, 12, 0))
		surface = cairo_image_surface_create(format, width, height);
	else
		surface = cairo_surface_create_similar_image(other, format, width, height);
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
#endif

#define N_FILTER 25
static double filter[N_FILTER];
static double total_duration;
static int frame_count;

void
fps_draw (cairo_t *cr,
	  const char *name,
	  const char *version,
	  const struct timeval *last,
	  const struct timeval *now)
{
    cairo_text_extents_t extents;
    char buf[80];
    double fps, avg;
    int n, max;

    fps = now->tv_sec - last->tv_sec;
    fps += (now->tv_usec - last->tv_usec) / 1000000.;
    total_duration += fps;

    max = N_FILTER;
    avg = fps;
    if (frame_count < max)
	max = frame_count;
    for (n = 0; n < max; n++)
	avg += filter[n];
    avg /= max + 1;
    filter[frame_count++ % N_FILTER] = fps;
    if (frame_count < 5) {
	    if (version)
		    snprintf (buf, sizeof (buf), "%s (%s): %d frame%s",
			      name, version, frame_count, frame_count == 1 ? "" : "s");
	    else
		    snprintf (buf, sizeof (buf), "%s: %d frame%s",
			      name, frame_count, frame_count == 1 ? "" : "s");
    } else {
	    if (version)
		    snprintf (buf, sizeof (buf), "%s (%s): %.1f fps, %d frames",
			      name, version, 1. / avg, frame_count);
	    else
		    snprintf (buf, sizeof (buf), "%s: %.1f fps, %d frames",
			      name, 1. / avg, frame_count);
    }
    cairo_set_font_size (cr, 18);
    cairo_text_extents (cr, buf, &extents);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_rectangle (cr, 4-1, 4-1, extents.width+2, extents.height+2);
    cairo_set_source_rgba (cr, .0, .0, .0, .5);
    cairo_fill (cr);

    cairo_move_to (cr, 4 - extents.x_bearing, 4 - extents.y_bearing);
    cairo_set_source_rgb (cr, .95, .95, .95);
    cairo_show_text (cr, buf);
}

void
fps_finish (struct framebuffer *fb,
	    const char *backend,
	    const char *version,
	    const char *name,
	    ...)
{
	cairo_t *cr = cairo_create(fb->surface);
	cairo_text_extents_t extents;
	char buf[80];
	double avg;
	va_list ap;

	if (frame_count == 0)
		return;

	avg = total_duration / frame_count;

	if (version)
		snprintf (buf, sizeof (buf), "%s (%s): %.1f fps, %d frames",
			  backend, version, 1. / avg, frame_count);
	else
		snprintf (buf, sizeof (buf), "%s: %.1f fps, %d frames",
			  backend, 1. / avg, frame_count);
	cairo_set_font_size (cr, 32);
	cairo_text_extents (cr, buf, &extents);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	cairo_rectangle (cr,
			 fb->device->width/2 - extents.width/2-4,
			 fb->device->height/2 - extents.height/2-4,
			 extents.width+8, extents.height+8);
	cairo_set_source_rgba (cr, .0, .0, .0, .5);
	cairo_fill (cr);

	cairo_move_to (cr,
		       fb->device->width/2 - extents.width/2,
		       fb->device->height/2 - extents.height/2 - extents.y_bearing);
	cairo_set_source_rgb (cr, .95, .95, .95);
	cairo_show_text (cr, buf);
	cairo_destroy (cr);

	va_start(ap, name);
	vfprintf (stdout, name, ap);
	va_end(ap);

	printf (": %.1f fps, %d frames, %.1f seconds\n",
		1. / avg, frame_count, total_duration);
	fflush (stdout);
}

#ifndef min
#define min(a, b) ((a) <= (b) ? (a) : (b))
#endif

enum clip device_get_clip(int argc, char **argv)
{
	const char *clip = NULL;
	enum clip c;
	int n;

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--clip") == 0 && n < argc - 1)
			clip = argv[++n];
		else if (strncmp (argv[n], "--clip=", 7) == 0)
			clip = argv[n] + 7;
	}

	c = CLIP_NONE;
	if (clip) {
		if (strcmp (clip, "none") == 0) {
		} else if (strcmp (clip, "region") == 0 || strcmp (clip, "region1") == 0) {
			c = CLIP_REGION1;
		} else if (strcmp (clip, "box") == 0 || strcmp (clip, "box1") == 0) {
			c = CLIP_BOX1;
		} else if (strcmp (clip, "region2") == 0) {
			c = CLIP_REGION2;
		} else if (strcmp (clip, "box2") == 0) {
			c = CLIP_BOX2;
		} else if (strcmp (clip, "diamond") == 0) {
			c = CLIP_DIAMOND;
		} else if (strcmp (clip, "circle") == 0) {
			c = CLIP_CIRCLE;
		}
	}

	return c;
}

void device_apply_clip(struct device *device, cairo_t *cr, enum clip c)
{
	cairo_new_path (cr);
	switch (c) {
	case CLIP_NONE:
		return;
	case CLIP_REGION1:
		cairo_rectangle (cr,
				 device->width/4, device->height/4,
				 device->width/2, device->height/2);
		break;
	case CLIP_BOX1:
		cairo_rectangle (cr,
				 device->width/4+.25, device->height/4+.25,
				 device->width/2, device->height/2);
		break;
	case CLIP_REGION2:
		cairo_rectangle (cr,
				 0, 0, 3*device->width/4, 3*device->height/4);
		cairo_rectangle (cr,
				 device->width/4, device->height/4,
				 3*device->width/4, 3*device->height/4);
		break;
	case CLIP_BOX2:
		cairo_rectangle (cr,
				 0.25, 0.25, 3*device->width/4, 3*device->height/4);
		cairo_rectangle (cr,
				 device->width/4-.25, device->height/4-.25,
				 3*device->width/4 + .25, 3*device->height/4 + .25);
		break;
	case CLIP_DIAMOND:
		cairo_move_to (cr, device->width/2, 0);
		cairo_line_to (cr, device->width, device->height/2);
		cairo_line_to (cr, device->width/2, device->height);
		cairo_line_to (cr, 0, device->height/2);
		cairo_close_path (cr);
		break;
	case CLIP_CIRCLE:
		cairo_arc (cr, device->width/2, device->height/2,
			   min(device->width/2, device->height/2),
			   0, 2 * M_PI);
		break;
	}
	cairo_clip (cr);
}
