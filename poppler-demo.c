#include "demo.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <glib-object.h>
#include <poppler.h>

static void
fps_draw (cairo_t *cr, const char *name, const struct timeval *last, const struct timeval *now)
{
#define N_FILTER 25
    static double filter[25];
    static int filter_pos;
    cairo_text_extents_t extents;
    char buf[80];
    double fps;
    int n, max;

    fps = now->tv_sec - last->tv_sec;
    fps += (now->tv_usec - last->tv_usec) / 1000000.;

    max = N_FILTER;
    if (filter_pos < max)
	max = filter_pos;
    for (n = 0; n < max; n++)
	fps += filter[n];
    fps /= max + 1;
    filter[filter_pos++ % N_FILTER] = fps;

    snprintf (buf, sizeof (buf), "%s: %.1f fps", name, 1. / fps);
    cairo_set_font_size (cr, 12);
    cairo_text_extents (cr, buf, &extents);
    cairo_move_to (cr, 4 - extents.x_bearing, 4 - extents.y_bearing);
    cairo_set_source_rgb (cr, .05, .05, .05);
    cairo_show_text (cr, buf);
}

int
main (int argc, char **argv)
{
    struct device *device;
    struct timeval last, now;
    enum {
	AUTO,
	XLIB,
	XIMAGE,
	XCB,
	GLX,
	DRM,
    } backend = AUTO;
    const char *filename = NULL;
    PopplerDocument *document;
    gchar *absolute, *uri;
    GError *error = NULL;
    int n, num_pages;

    g_type_init ();

    for (n = 1; n < argc; n++) {
	if (strcmp (argv[n], "--xlib") == 0) {
	    backend = XLIB;
	} else if (strcmp (argv[n], "--xcb") == 0) {
	    backend = XCB;
	} else if (strcmp (argv[n], "--ximage") == 0) {
	    backend = XIMAGE;
	} else if (strcmp (argv[n], "--glx") == 0) {
	    backend = GLX;
	} else if (strcmp (argv[n], "--drm") == 0) {
	    backend = DRM;
	} else if (strcmp (argv[n], "--filename") == 0) {
	    filename = argv[n+1];
	    n++;
	}
    }

    if (filename == NULL) {
	fprintf (stderr, "Please use --filename <filename> to specify the PDF file to render\n");
	return 1;
    }

    if (g_path_is_absolute (filename)) {
	absolute = g_strdup (filename);
    } else {
	gchar *dir = g_get_current_dir ();
	absolute = g_build_filename (dir, filename, (gchar *) 0);
	g_free (dir);
    }

    uri = g_filename_to_uri (absolute, NULL, &error);
    g_free (absolute);
    if (uri == NULL)
	return 1;

    document = poppler_document_new_from_file (uri, NULL, &error);
    g_free (uri);
    if (document == NULL)
	return 1;

    num_pages = poppler_document_get_n_pages (document);

    if (backend == AUTO) {
	device = NULL;
	if (device == NULL && HAVE_DRM) {
	    device = drm_open (argc, argv);
	}
	if (device == NULL && HAVE_XCB)
	    device = xcb_open (argc, argv);
	if (device == NULL && HAVE_XLIB)
	    device = xlib_open (argc, argv);
	if (device == NULL && HAVE_XIMAGE)
	    device = ximage_open (argc, argv);
	if (device == NULL && HAVE_GLX)
	    device = glx_open (argc, argv);
	if (device == NULL) {
	    fprintf (stderr, "Failed to open output device.\n");
	    return 1;
	}
    } else {
	switch (backend) {
	case AUTO:
	case XLIB:
#if HAVE_XLIB
	    device = xlib_open (argc, argv);
#endif
	    break;
	case XCB:
#if HAVE_XCB
	    device = xcb_open (argc, argv);
#endif
	    break;
	case XIMAGE:
#if HAVE_XIMAGE
	    device = ximage_open (argc, argv);
#endif
	    break;
	case DRM:
#if HAVE_DRM
	    device = drm_open (argc, argv);
#endif
	    break;
	case GLX:
#if HAVE_GLX
	    device = glx_open (argc, argv);
#endif
	    break;
	}

	if (device == NULL) {
	    fprintf (stderr, "Failed to open backend device\n");
	    return 1;
	}
    }

    n = 0;
    gettimeofday (&last, NULL);
    while (1) {
	cairo_t *cr;
	struct framebuffer *fb;
	PopplerPage *page;
	double page_width, page_height;
	double sf_x, sf_y, sf;

	fb = device->get_framebuffer (device);

	page = poppler_document_get_page (document, n++ % num_pages);

	cr = cairo_create (fb->surface);

	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);

	poppler_page_get_size (page, &page_width, &page_height);
	sf_x = device->width / page_width;
	sf_y = device->height / page_height;
	sf = MIN (sf_x, sf_y);
	cairo_scale (cr, sf, sf);

	poppler_page_render (page, cr);
	g_object_unref (page);

	gettimeofday (&now, NULL);
	fps_draw (cr, device->name, &last, &now);
	last = now;

	cairo_destroy (cr);

	fb->show (fb);
	fb->destroy (fb);
    }

    return 0;
}
