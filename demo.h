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


struct device *xcb_open (int argc, char **argv);
struct device *xlib_open (int argc, char **argv);
struct device *ximage_open (int argc, char **argv);
struct device *glx_open (int argc, char **argv);
struct device *drm_open (int argc, char **argv);
