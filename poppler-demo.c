#include "demo.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <glib-object.h>
#include <poppler.h>

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int
main (int argc, char **argv)
{
    struct device *device;
    struct timeval last, now;
    const char *filename = NULL;
    const char *version;
    PopplerDocument *document;
    gchar *absolute, *uri;
    GError *error = NULL;
    int n, num_pages;
    int benchmark;

    device = device_open(argc, argv);
    version = device_show_version(argc, argv);
    benchmark = device_get_benchmark(argc, argv);
    if (benchmark == 0)
	    benchmark = 20;

    for (n = 1; n < argc; n++) {
	if (strcmp (argv[n], "--filename") == 0) {
	    filename = argv[n+1];
	    n++;
	}
    }
    if (filename == NULL) {
	fprintf (stderr, "Please use --filename <filename> to specify the PDF file to render\n");
	return 1;
    }


    g_type_init ();

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

    signal (SIGHUP, signal_handler);

    num_pages = poppler_document_get_n_pages (document);

    n = 0;
    gettimeofday (&last, NULL);
    do {
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
	cairo_save(cr);
	cairo_scale (cr, sf, sf);

	poppler_page_render (page, cr);
	cairo_restore(cr);
	g_object_unref (page);

	gettimeofday (&now, NULL);
	fps_draw (cr, device->name, version, &last, &now);
	last = now;

	cairo_destroy (cr);

	fb->show (fb);
	fb->destroy (fb);
    } while (!done);

    if (benchmark < 0) {
	    struct framebuffer *fb = device->get_framebuffer (device);
	    cairo_t *cr = cairo_create(fb->surface);
	    PopplerPage *page = poppler_document_get_page (document, 0);
	    double page_width, page_height;
	    double sf_x, sf_y, sf;

	    cairo_set_source_rgb (cr, 1, 1, 1);
	    cairo_paint (cr);

	    poppler_page_get_size (page, &page_width, &page_height);
	    sf_x = device->width / page_width;
	    sf_y = device->height / page_height;
	    sf = MIN (sf_x, sf_y);
	    cairo_scale (cr, sf, sf);
	    poppler_page_render (page, cr);
	    g_object_unref (page);

	    cairo_destroy(cr);

	    fps_finish(fb, device->name, version);
	    fb->show (fb);
	    fb->destroy (fb);
	    pause();
    }

    return 0;
}
