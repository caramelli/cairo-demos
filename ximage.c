#include <cairo-xlib.h>

#include "demo.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ximage_device {
    struct device base;
    struct framebuffer fb;
};

static void
destroy (struct framebuffer *abstract_framebuffer)
{
}

static void flush (struct ximage_device *device)
{
    cairo_surface_flush (device->base.scanout);
    XFlush (cairo_xlib_surface_get_display (device->base.scanout));
}

static void
show (struct framebuffer *fb)
{
    struct ximage_device *device = (struct ximage_device *) fb->device;
    cairo_t *cr;

    cr = cairo_create (device->base.scanout);
    cairo_set_source_surface (cr, fb->surface, 0, 0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);

    flush (device);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct ximage_device *device = (struct ximage_device *) abstract_device;
    return &device->fb;
}

static void
inplace_show (struct framebuffer *fb)
{
    struct ximage_device *device = (struct ximage_device *) fb->device;
    cairo_surface_unmap_image (device->base.scanout, fb->surface);

    flush (device);
}

static struct framebuffer *
get_inplace_fb (struct device *abstract_device)
{
    struct ximage_device *device = (struct ximage_device *) abstract_device;
    device->fb.surface = cairo_surface_map_to_image (device->base.scanout, NULL);
    return &device->fb;
}

struct device *
ximage_open (int argc, char **argv)
{
    struct ximage_device *device;
    Display *dpy;
    Screen *scr;
    Window win;
    int screen;
    XSetWindowAttributes attr;
    int i, similar, map;
    int x, y;

    similar = 0;
    map = 0;
    for (i = 1; i < argc; i++) {
	    if (strcmp (argv[i], "--similar") == 0)
		    similar = cairo_version () >= CAIRO_VERSION_ENCODE (1, 12, 0);
	    else if (strcmp (argv[i], "--map") == 0)
		    map = cairo_version () >= CAIRO_VERSION_ENCODE (1, 12, 0);
    }

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
	return NULL;

    device = (struct ximage_device *) malloc (sizeof (struct ximage_device));
    device->base.name = "ximage";

    screen = DefaultScreen (dpy);
    scr = XScreenOfDisplay (dpy, screen);
    device->base.width = WidthOfScreen (scr);
    device->base.height = HeightOfScreen (scr);
    device_get_size (argc, argv, &device->base.width, &device->base.height);
    x = y = 0;
    switch (device_get_split(argc, argv)) {
    case SPLIT_NONE:
	    break;
    case SPLIT_LEFT:
	    device->base.width /= 2;
	    break;
    case SPLIT_RIGHT:
	    x = device->base.width /= 2;
	    break;
    case SPLIT_TOP:
	    device->base.height /= 2;
	    break;
    case SPLIT_BOTTOM:
	    y = device->base.height /= 2;
	    break;

    case SPLIT_BOTTOM_LEFT:
	    device->base.width /= 2;
	    y = device->base.height /= 2;
	    break;
    case SPLIT_BOTTOM_RIGHT:
	    x = device->base.width /= 2;
	    y = device->base.height /= 2;
	    break;

    case SPLIT_TOP_LEFT:
	    device->base.width /= 2;
	    device->base.height /= 2;
	    break;
    case SPLIT_TOP_RIGHT:
	    x = device->base.width /= 2;
	    device->base.height /= 2;
	    break;
    }

    attr.override_redirect = True;
    win = XCreateWindow (dpy, DefaultRootWindow (dpy),
			 x, y,
			 device->base.width, device->base.height, 0,
			 DefaultDepth (dpy, screen),
			 InputOutput,
			 DefaultVisual (dpy, screen),
			 CWOverrideRedirect, &attr);
    XMapWindow (dpy, win);
    XFlush (dpy);

    device->base.scanout =
	    cairo_xlib_surface_create (dpy, win,
				       DefaultVisual (dpy, screen),
				       device->base.width, device->base.height);

    if (map) {
	    device->fb.show = inplace_show;
	    device->base.get_framebuffer = get_inplace_fb;
    } else {
	    if (similar)
		    device->fb.surface =
			    cairo_surface_create_similar_image (device->base.scanout,
								CAIRO_FORMAT_RGB24,
								device->base.width, device->base.height);
	    else
		    device->fb.surface =
			    cairo_image_surface_create (CAIRO_FORMAT_RGB24,
							device->base.width, device->base.height);

	    device->fb.show = show;
	    device->base.get_framebuffer = get_fb;
    }
    device->fb.destroy = destroy;
    device->fb.device = &device->base;

    return &device->base;
}
