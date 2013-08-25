#include <cairo-xlib.h>

#include "demo.h"

#include <stdlib.h>

#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shmproto.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct xlib_device {
    struct device base;

    int q;
    struct framebuffer fb[2];

    Display *display;
    Window drawable;
    Pixmap pixmap[2];
    GC gc;

    struct {
	    XShmSegmentInfo shm;
	    Pixmap pixmap;
	    int opcode, event;
    } shm_sync;
};

static void shm_sync_event (struct xlib_device *device)
{
	XShmCompletionEvent ev;

	ev.type = device->shm_sync.event;
	ev.send_event = 1; /* XXX or lie? */
	ev.serial = NextRequest (device->display);
	ev.drawable = device->drawable;
	ev.major_code = device->shm_sync.opcode;
	ev.minor_code = X_ShmPutImage;
	ev.shmseg = device->shm_sync.shm.shmid;
	ev.offset = device->q;

	XSendEvent (device->display, ev.drawable, False, 0, (XEvent *)&ev);
	XFlush (device->display);
}

static void shm_sync_init (struct xlib_device *device)
{
    Display *dpy = device->display;
    int major, minor, has_pixmap;
    XExtCodes *codes;

    XShmQueryVersion (dpy, &major, &minor, &has_pixmap);
    if (!has_pixmap)
	return;

    codes = XInitExtension (dpy, SHMNAME);
    if (codes == NULL)
	return;

    device->shm_sync.shm.shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0777);
    device->shm_sync.shm.shmaddr = (char *) shmat (device->shm_sync.shm.shmid, NULL, 0);
    device->shm_sync.shm.readOnly = False;

    if (!XShmAttach (dpy, &device->shm_sync.shm))
	abort ();

    device->shm_sync.opcode = codes ->major_opcode;
    device->shm_sync.event = codes->first_event;

    device->shm_sync.pixmap = XShmCreatePixmap (dpy,
						device->drawable,
						device->shm_sync.shm.shmaddr,
						&device->shm_sync.shm,
						1, 1, 24);

    shm_sync_event (device);
}

static void shm_sync (struct xlib_device *device)
{
    if (!device->shm_sync.pixmap)
	return;

    do {
	XEvent ev;
	XShmCompletionEvent *shm;

	XNextEvent (device->display, &ev);
	if (ev.type != device->shm_sync.event)
	    continue;

	shm = (XShmCompletionEvent *)&ev;
	if (shm->shmseg != device->shm_sync.shm.shmid)
	    continue;

	return;
    } while (1);
}

static void
destroy (struct framebuffer *fb)
{
}

static void
show (struct framebuffer *fb)
{
    struct xlib_device *device = (struct xlib_device *) fb->device;

    cairo_surface_flush (fb->surface);

    XCopyArea(device->display,
	      device->pixmap[device->q&1],
	      device->drawable,
	      device->gc,
	      0, 0,
	      device->base.width, device->base.height,
	      0, 0);

    XFlush (device->display);
    shm_sync(device);
}

static struct framebuffer *
get_fb (struct device *abstract_device)
{
    struct xlib_device *device = (struct xlib_device *) abstract_device;
    int q = ++device->q & 1;
    if (device->shm_sync.pixmap) {
	XCopyArea(device->display,
		  device->pixmap[q],
		  device->shm_sync.pixmap,
		  device->gc,
		  0, 0,
		  1, 1,
		  0, 0);
	shm_sync_event (device);
    } else {
	XImage *image = XGetImage(device->display,
				  device->pixmap[q],
				  0, 0, 1, 1,
				  AllPlanes, ZPixmap);
	if (image)
	    XDestroyImage(image);
    }
    return &device->fb[q];
}

struct device *
xlib_open (int argc, char **argv)
{
    struct xlib_device *device;
    Display *dpy;
    Screen *scr;
    int screen;
    XSetWindowAttributes attr;
    int x, y;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
	return NULL;

    device = malloc (sizeof (struct xlib_device));
    device->base.name = "xlib";
    device->base.get_framebuffer = get_fb;
    device->display = dpy;
    device->q = 0;

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

    device->base.scanout = cairo_xlib_surface_create (dpy, device->drawable,
						      DefaultVisual (dpy, screen),
						      device->base.width, device->base.height);

    device->pixmap[0] = XCreatePixmap(dpy, device->drawable,
				      device->base.width, device->base.height,
				      DefaultDepth (dpy, screen));
    device->fb[0].surface = cairo_xlib_surface_create (dpy, device->pixmap[0],
						    DefaultVisual (dpy, screen),
						    device->base.width, device->base.height);

    device->pixmap[1] = XCreatePixmap(dpy, device->drawable,
				      device->base.width, device->base.height,
				      DefaultDepth (dpy, screen));
    device->fb[1].surface = cairo_xlib_surface_create (dpy, device->pixmap[1],
						    DefaultVisual (dpy, screen),
						    device->base.width, device->base.height);

    device->fb[1].device = device->fb[0].device = &device->base;
    device->fb[1].show = device->fb[0].show = show;
    device->fb[1].destroy = device->fb[0].destroy = destroy;

    {
	    XGCValues gcv;

	    gcv.graphics_exposures = False;
	    device->gc = XCreateGC(dpy, device->drawable,
				   GCGraphicsExposures, &gcv);
    }

    shm_sync_init (device);

    return &device->base;
}
