#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "demo.h"

struct device *device_open(int argc, char **argv)
{
    struct device *device = 0;
    enum {
	AUTO,
	XLIB,
	XIMAGE,
	XCB,
	GLX,
	DRM,
    } backend = AUTO;
    int n;

    for (n = 1; n < argc; n++) {
	if (strcmp (argv[n], "--xlib") == 0) {
	    backend = XLIB;
	} else if (strcmp (argv[n], "--xcb") == 0) {
	    backend = XCB;
	} else if (strcmp (argv[n], "--ximage") == 0) {
	    backend = XIMAGE;
	} else if (strcmp (argv[n], "--drm") == 0) {
	    backend = DRM;
	} else if (strcmp (argv[n], "--glx") == 0) {
	    backend = GLX;
	}
    }

    if (backend == AUTO) {
	if (device == 0 && HAVE_DRM)
	    device = drm_open (argc, argv);
	if (device == 0 && HAVE_XCB)
	    device = xcb_open (argc, argv);
	if (device == 0 && HAVE_XLIB)
	    device = xlib_open (argc, argv);
	if (device == 0 && HAVE_XLIB)
	    device = glx_open (argc, argv);
	if (device == 0 && HAVE_XIMAGE)
	    device = ximage_open (argc, argv);
    } else switch (backend) {
    case AUTO:
    case XLIB:
	device = xlib_open (argc, argv);
	break;
    case XCB:
	device = xcb_open (argc, argv);
	break;
    case XIMAGE:
	device = ximage_open (argc, argv);
	break;
    case GLX:
	device = glx_open (argc, argv);
	break;
    case DRM:
#if HAVE_DRM
	device = drm_open (argc, argv);
#endif
	break;
    }

    if (device == 0) {
	fprintf(stderr, "Failed to open a drawing device\n");
	exit(1);
    }

    printf("Using backend \"%s\"\n", device->name);
    return device;
}

