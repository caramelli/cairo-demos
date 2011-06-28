#include <cairo-xcb.h>

#include "demo.h"

#include <stdlib.h>

struct xcb_device {
    struct device base;

    xcb_connection_t *connection;
    xcb_window_t drawable;
};

struct xcb_framebuffer {
    struct framebuffer base;
};

static void
destroy (struct framebuffer *abstract_framebuffer)
{
    struct xcb_framebuffer *fb = (struct xcb_framebuffer *) abstract_framebuffer;

    cairo_surface_destroy (fb->base.surface);
    free (fb);
}

static void
show (struct framebuffer *abstract_framebuffer)
{
    struct xcb_framebuffer *fb = (struct xcb_framebuffer *) abstract_framebuffer;
    struct xcb_device *device = (struct xcb_device *) fb->base.device;
    cairo_t *cr;

    cr = cairo_create (device->base.scanout);
    cairo_set_source_surface (cr, fb->base.surface, 0, 0);
    cairo_paint (cr);
    cairo_destroy (cr);

    xcb_flush (device->connection);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct xcb_device *device = (struct xcb_device *) abstract_device;
    struct xcb_framebuffer *fb;

    fb = malloc (sizeof (struct xcb_framebuffer));
    fb->base.device = &device->base;
    fb->base.show = show;
    fb->base.destroy = destroy;

    fb->base.surface = cairo_surface_create_similar (device->base.scanout,
						     CAIRO_CONTENT_COLOR,
						     device->base.width,
						     device->base.height);

    return &fb->base;
}

static xcb_visualtype_t *
lookup_visual (xcb_screen_t *s, xcb_visualid_t visual)
{
    xcb_depth_iterator_t d;

    d = xcb_screen_allowed_depths_iterator (s);
    for (; d.rem; xcb_depth_next (&d)) {
	xcb_visualtype_iterator_t v = xcb_depth_visuals_iterator (d.data);
	for (; v.rem; xcb_visualtype_next (&v)) {
	    if (v.data->visual_id == visual)
		return v.data;
	}
    }

    return 0;
}

struct device *
xcb_open (int argc, char **argv)
{
    struct xcb_device *device;
    xcb_connection_t *c;
    xcb_screen_t *s;
    xcb_visualtype_t *v;
    uint32_t values[] = { 1 };

    c = xcb_connect (NULL, NULL);
    if (c == NULL)
	return NULL;

    device = malloc (sizeof (struct xcb_device));
    device->base.name = "xcb";
    device->base.get_framebuffer = get_fb;
    device->connection = c;
    device->drawable = xcb_generate_id (c);

    s = xcb_setup_roots_iterator (xcb_get_setup (c)).data;
    v = lookup_visual (s, s->root_visual);

    device->base.width = s->width_in_pixels;
    device->base.height = s->height_in_pixels;

    xcb_create_window (c,
		       s->root_depth,
		       device->drawable,
		       s->root,
		       0, 0, device->base.width, device->base.height, 0,
		       XCB_WINDOW_CLASS_INPUT_OUTPUT,
		       s->root_visual,
		       XCB_CW_OVERRIDE_REDIRECT,
		       values);
    xcb_map_window (c, device->drawable);

    device->base.scanout = cairo_xcb_surface_create (c,
						     device->drawable,
						     v,
						     device->base.width,
						     device->base.height);

    return &device->base;
}

