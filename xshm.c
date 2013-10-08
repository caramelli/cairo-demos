#include <cairo-xlib.h>

#include "demo.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct xshm_device {
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
    struct xshm_device *device = (struct xshm_device *) fb->device;

    if (device->pixmap) {
	XCopyArea (device->display, device->pixmap, device->drawable, device->gc,
		   0, 0,
		   device->base.width, device->base.height,
		   0, 0);
    } else {
	XImage xshm;
	int native_byte_order = _native_byte_order_lsb () ? LSBFirst : MSBFirst;

	xshm.width = device->base.width;
	xshm.height = device->base.height;
	xshm.format = ZPixmap;
	xshm.byte_order = native_byte_order;
	xshm.bitmap_unit = 32;	/* always for libpixman */
	xshm.bitmap_bit_order = native_byte_order;
	xshm.bitmap_pad = 32;	/* always for libpixman */
	xshm.depth = 24;
	xshm.red_mask = 0xff;
	xshm.green_mask = 0xff00;
	xshm.blue_mask = 0xff000;
	xshm.xoffset = 0;
	xshm.bits_per_pixel = 32;
	xshm.data = device->shm.shmaddr;
	xshm.obdata = (char *) &device->shm;
	xshm.bytes_per_line = device->stride;

	XShmPutImage (device->display, device->drawable, device->gc, &xshm,
		      0, 0, 0, 0, device->base.width, device->base.height,
		      False);
    }

    XSync (device->display, True);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct xshm_device *device = (struct xshm_device *) abstract_device;
    return &device->fb;
}

struct device *
xshm_open (int argc, char **argv)
{
    struct xshm_device *device;
    Display *dpy;
    Screen *scr;
    int screen;
    XSetWindowAttributes attr;
    enum split split;
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

    device = (struct xshm_device *) malloc (sizeof (struct xshm_device));
    device->base.name = "xshm";
    device->base.get_framebuffer = get_fb;
    device->display = dpy;

    screen = DefaultScreen (dpy);
    scr = XScreenOfDisplay (dpy, screen);
    device->base.width = WidthOfScreen (scr);
    device->base.height = HeightOfScreen (scr);
    device_get_size (argc, argv, &device->base.width, &device->base.height);
    x = y = 0;
    switch ((split = device_get_split(argc, argv))) {
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

    case SPLIT_1_16...SPLIT_16_16:
	    split -= SPLIT_1_16;
	    x = split & 3;
	    y = (split >> 2) & 3;
	    device->base.width /= 4;
	    device->base.height /= 4;
	    x *= device->base.width;
	    y *= device->base.height;
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
    device->shm.shmaddr = (char *) shmat (device->shm.shmid, NULL, 0);
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
