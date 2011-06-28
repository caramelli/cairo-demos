#include <cairo-gl.h>

#include "demo.h"

#include <stdlib.h>
#include <X11/X.h>

struct glx_device {
    struct device base;
    struct framebuffer fb;

    Display *display;
    Window drawable;
    GLXContext ctx;
    cairo_device_t *device;
};

static void
destroy (struct framebuffer *fb)
{
}

static void
show (struct framebuffer *fb)
{
    cairo_gl_surface_swapbuffers (fb->surface);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct glx_device *device = (struct glx_device *) abstract_device;

    return &device->fb;
}

struct device *
glx_open (int argc, char **argv)
{
	int rgba_attribs[] = {
	       	GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_DOUBLEBUFFER,
		None };
	XVisualInfo *vi;
	struct glx_device *device;
	Display *dpy;
	Screen *scr;
	int screen;
	XSetWindowAttributes attr;

	dpy = XOpenDisplay (NULL);
	if (dpy == NULL)
		return NULL;

	device = malloc(sizeof(struct glx_device));
	device->base.name = "glx";
	device->base.get_framebuffer = get_fb;
	device->display = dpy;

	device->fb.device = &device->base;
	device->fb.show = show;
	device->fb.destroy = destroy;

	screen = DefaultScreen (dpy);
	scr = XScreenOfDisplay (dpy, screen);
	device->base.width = WidthOfScreen (scr);
	device->base.height = HeightOfScreen (scr);

	vi = glXChooseVisual (dpy, DefaultScreen (dpy), rgba_attribs);
	if (vi == NULL) {
		XCloseDisplay (dpy);
		free (device);
		return NULL;
	}

	attr.colormap = XCreateColormap (dpy,
					 RootWindow (dpy, vi->screen),
					 vi->visual,
					 AllocNone);

	attr.border_pixel = 0;
	attr.override_redirect = True;
	device->drawable = XCreateWindow (dpy, DefaultRootWindow (dpy),
					  0, 0,
					  device->base.width, device->base.height, 0,
					  vi->depth, InputOutput, vi->visual,
					  CWOverrideRedirect | CWBorderPixel | CWColormap,
					  &attr);
	XMapWindow (dpy, device->drawable);

	device->ctx = glXCreateContext(dpy, vi, NULL, True);
	XFree(vi);

	device->device = cairo_glx_device_create(dpy, device->ctx);

	device->base.scanout =
	       	cairo_gl_surface_create_for_window (device->device,
						    device->drawable,
						    device->base.width,
						    device->base.height);
	device->fb.surface = device->base.scanout;

	return &device->base;
}
