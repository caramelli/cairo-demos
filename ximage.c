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

    Display *display;
    Window drawable;

    int stride;
    Pixmap pixmap;
    GC gc;
    XShmSegmentInfo shm;
};

static void
destroy (struct framebuffer *abstract_framebuffer)
{
}

static cairo_bool_t
_native_byte_order_lsb (void)
{
    int	x = 1;

    return *((char *) &x) == 1;
}

static void
show (struct framebuffer *fb)
{
    struct ximage_device *device = (struct ximage_device *) fb->device;

    if (device->pixmap) {
	XCopyArea (device->display, device->pixmap, device->drawable, device->gc,
		   0, 0,
		   device->base.width, device->base.height,
		   0, 0);
    } else {
	XImage ximage;
	int native_byte_order = _native_byte_order_lsb () ? LSBFirst : MSBFirst;

	ximage.width = device->base.width;
	ximage.height = device->base.height;
	ximage.format = ZPixmap;
	ximage.byte_order = native_byte_order;
	ximage.bitmap_unit = 32;	/* always for libpixman */
	ximage.bitmap_bit_order = native_byte_order;
	ximage.bitmap_pad = 32;	/* always for libpixman */
	ximage.depth = 24;
	ximage.red_mask = 0xff;
	ximage.green_mask = 0xff00;
	ximage.blue_mask = 0xff000;
	ximage.xoffset = 0;
	ximage.bits_per_pixel = 32;
	ximage.data = device->shm.shmaddr;
	ximage.obdata = (char *) &device->shm;
	ximage.bytes_per_line = device->stride;

	XShmPutImage (device->display, device->drawable, device->gc, &ximage,
		      0, 0, 0, 0, device->base.width, device->base.height,
		      False);
    }

    XSync (device->display, True);
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
    int screen;
    XSetWindowAttributes attr;
    int major, minor, has_pixmap;
    int x, y;
    XGCValues gcv;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
	return NULL;

    if (! XShmQueryExtension (dpy)) {
	XCloseDisplay (dpy);
	return NULL;
    }

    device = malloc (sizeof (struct ximage_device));
    device->base.name = "ximage";
    device->base.get_framebuffer = get_fb;
    device->display = dpy;

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
    device->drawable = XCreateWindow (dpy, DefaultRootWindow (dpy),
				      x, y,
				      device->base.width, device->base.height, 0,
				      DefaultDepth (dpy, screen),
				      InputOutput,
				      DefaultVisual (dpy, screen),
				      CWOverrideRedirect, &attr);
    XMapWindow (dpy, device->drawable);

    device->stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, device->base.width);

    device->shm.shmid = shmget(IPC_PRIVATE, device->stride * device->base.height, IPC_CREAT | 0777);
    device->shm.shmaddr = shmat (device->shm.shmid, NULL, 0);
    device->shm.readOnly = False;

    if (!XShmAttach (dpy, &device->shm))
	abort ();

    device->base.scanout = cairo_image_surface_create_for_data ((uint8_t *) device->shm.shmaddr,
								CAIRO_FORMAT_RGB24,
								device->base.width,
								device->base.height,
								device->stride);
    XShmQueryVersion (dpy, &major, &minor, &has_pixmap);

    if (has_pixmap) {
	printf ("Using SHM Pixmap\n");
	device->pixmap = XShmCreatePixmap (dpy,
					   device->drawable,
					   device->shm.shmaddr,
					   &device->shm,
					   device->base.width,
					   device->base.height,
					   24);
    } else {
	printf ("Using SHM PutImage\n");
	device->pixmap = 0;
    }

    gcv.graphics_exposures = False;
    device->gc = XCreateGC (dpy, device->drawable, GCGraphicsExposures, &gcv);

    device->fb.device = &device->base;
    device->fb.show = show;
    device->fb.destroy = destroy;
    device->fb.surface = device->base.scanout;

    return &device->base;
}
