#include <cairo-xlib.h>

#include "demo.h"

#include <stdlib.h>

struct xlib_device {
    struct device base;

    Display *display;
    Window drawable;
};

struct xlib_framebuffer {
    struct framebuffer base;
};

static void
destroy (struct framebuffer *abstract_framebuffer)
{
    struct xlib_framebuffer *fb = (struct xlib_framebuffer *) abstract_framebuffer;

    cairo_surface_destroy (fb->base.surface);
    free (fb);
}

static void
show (struct framebuffer *abstract_framebuffer)
{
    struct xlib_framebuffer *fb = (struct xlib_framebuffer *) abstract_framebuffer;
    struct xlib_device *device = (struct xlib_device *) fb->base.device;
    cairo_t *cr;

    cr = cairo_create (device->base.scanout);
    cairo_set_source_surface (cr, fb->base.surface, 0, 0);
    cairo_paint (cr);
    cairo_destroy (cr);

    XSync (device->display, True);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct xlib_device *device = (struct xlib_device *) abstract_device;
    struct xlib_framebuffer *fb;

    fb = malloc (sizeof (struct xlib_framebuffer));
    fb->base.device = &device->base;
    fb->base.show = show;
    fb->base.destroy = destroy;

    fb->base.surface = cairo_surface_create_similar (device->base.scanout,
						     CAIRO_CONTENT_COLOR,
						     device->base.width,
						     device->base.height);

    return &fb->base;
}

struct device *
xlib_open (int argc, char **argv)
{
    struct xlib_device *device;
    Display *dpy;
    Screen *scr;
    int screen;
    XSetWindowAttributes attr;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
	return NULL;

    device = malloc (sizeof (struct xlib_device));
    device->base.name = "xlib";
    device->base.get_framebuffer = get_fb;
    device->display = dpy;

    screen = DefaultScreen (dpy);
    scr = XScreenOfDisplay (dpy, screen);
    device->base.width = WidthOfScreen (scr);
    device->base.height = HeightOfScreen (scr);

    attr.override_redirect = True;
    device->drawable = XCreateWindow (dpy, DefaultRootWindow (dpy),
				      0, 0,
				      device->base.width, device->base.height, 0,
				      DefaultDepth (dpy, screen),
				      InputOutput,
				      DefaultVisual (dpy, screen),
				      CWOverrideRedirect, &attr);
    XMapWindow (dpy, device->drawable);

    device->base.scanout = cairo_xlib_surface_create (dpy, device->drawable,
						      DefaultVisual (dpy, screen),
						      device->base.width, device->base.height);

    return &device->base;
}
