#include <stdint.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
const char *device_show_version(int argc, char **argv);
int device_get_size(int argc, char **argv, int *width, int *height);
enum split {
	SPLIT_NONE = 0,
	SPLIT_LEFT,
	SPLIT_RIGHT,
	SPLIT_TOP,
	SPLIT_BOTTOM,
	SPLIT_TOP_LEFT,
	SPLIT_TOP_RIGHT,
	SPLIT_BOTTOM_LEFT,
	SPLIT_BOTTOM_RIGHT,
} device_get_split(int argc, char **argv);

cairo_antialias_t device_get_antialias(int argc, char **argv);
int device_get_benchmark(int argc, char **argv);
enum clip {
	CLIP_NONE,
	CLIP_REGION1,
	CLIP_BOX1,
	CLIP_REGION2,
	CLIP_BOX2,
	CLIP_DIAMOND,
	CLIP_CIRCLE,
} device_get_clip(int argc, char **argv);
void device_apply_clip(struct device *device, cairo_t *cr, enum clip c);

#if HAVE_COGL
struct device *cogl_open (int argc, char **argv);
#else
static inline struct device *cogl_open (int argc, char **argv) { return 0; }
#endif

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

cairo_surface_t *
_cairo_image_surface_create_from_pixbuf(const GdkPixbuf *pixbuf);

void
fps_draw (cairo_t *cr, const char *name, const char *version,
	  const struct timeval *last,
	  const struct timeval *now);
void
fps_finish (struct framebuffer *fb,
	    const char *name,
	    const char *version);
