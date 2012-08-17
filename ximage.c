#include <cairo-xlib.h>

#include "demo.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct ximage_device {
    struct device base;
    struct framebuffer fb;
};

static void
destroy (struct framebuffer *abstract_framebuffer)
{
}

static void
show (struct framebuffer *fb)
{
    struct ximage_device *device = (struct ximage_device *) fb->device;
    cairo_t *cr = cairo_create (device->base.scanout);
    cairo_set_source_surface (cr, fb->surface, 0, 0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct ximage_device *device = (struct ximage_device *) abstract_device;
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
    int x, y;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
	return NULL;

    device = (struct ximage_device *) malloc (sizeof (struct ximage_device));
    device->base.name = "ximage";
    device->base.get_framebuffer = get_fb;

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

    device->base.scanout = cairo_xlib_surface_create (dpy, win,
						      DefaultVisual (dpy, screen),
						      device->base.width, device->base.height);

    device->fb.surface =
	    cairo_surface_create_similar_image (device->base.scanout,
						CAIRO_FORMAT_RGB24,
						device->base.width, device->base.height);

    device->fb.device = &device->base;
    device->fb.show = show;
    device->fb.destroy = destroy;

    return &device->base;
}
