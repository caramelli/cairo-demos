#include <cairo.h>
#include <stdint.h>

struct device;
struct framebuffer;
struct slide;

struct framebuffer {
    struct device *device;

    cairo_surface_t *surface;

    void (*show) (struct framebuffer *);
    void (*destroy) (struct framebuffer *);
};

struct device {
    const char *name;
    struct framebuffer *(*get_framebuffer) (struct device *);
    cairo_surface_t *scanout;
    int width, height;
};

struct slide {
    void (*draw) (struct slide *, cairo_t *, int, int);
};

struct device *device_open(int argc, char **argv);

#if HAVE_GLX
struct device *glx_open (int argc, char **argv);
#else
static inline struct device *glx_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_XCB
struct device *xcb_open (int argc, char **argv);
#else
static inline struct device *xcb_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_XLIB
struct device *xlib_open (int argc, char **argv);
#else
static inline struct device *xlib_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_XIMAGE
struct device *ximage_open (int argc, char **argv);
#else
static inline struct device *ximage_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_DRM
struct device *drm_open (int argc, char **argv);
#else
static inline struct device *drm_open (int argc, char **argv) { return 0; }
#endif
