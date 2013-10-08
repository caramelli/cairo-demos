#include <stdint.h>
#include <sys/time.h>

#include <cairo.h>

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

	SPLIT_1_16 = 0x10,
	SPLIT_2_16,
	SPLIT_3_16,
	SPLIT_4_16,
	SPLIT_5_16,
	SPLIT_6_16,
	SPLIT_7_16,
	SPLIT_8_16,
	SPLIT_9_16,
	SPLIT_10_16,
	SPLIT_11_16,
	SPLIT_12_16,
	SPLIT_13_16,
	SPLIT_14_16,
	SPLIT_15_16,
	SPLIT_16_16,
};
enum split device_get_split(int argc, char **argv);

cairo_antialias_t device_get_antialias(int argc, char **argv);
const char *device_antialias_to_string(cairo_antialias_t antialias);

int device_get_benchmark(int argc, char **argv);
enum clip {
	CLIP_NONE,
	CLIP_REGION1,
	CLIP_BOX1,
	CLIP_REGION2,
	CLIP_BOX2,
	CLIP_DIAMOND,
	CLIP_CIRCLE,
};
enum clip device_get_clip(int argc, char **argv);
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

#if HAVE_XSHM
struct device *xshm_open (int argc, char **argv);
#else
static inline struct device *xshm_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_DRM
struct device *drm_open (int argc, char **argv);
#else
static inline struct device *drm_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_SKIA
struct device *skia_open (int argc, char **argv);
#else
static inline struct device *skia_open (int argc, char **argv) { return 0; }
#endif

#if HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>

cairo_surface_t *
surface_create_from_pixbuf(cairo_surface_t *other, const GdkPixbuf *pixbuf);
#endif

cairo_surface_t *
surface_create_from_png (cairo_surface_t *other, const char *filename);

void
fps_draw (cairo_t *cr, const char *name, const char *version,
	  const struct timeval *last,
	  const struct timeval *now);
void
fps_finish (struct framebuffer *fb,
	    const char *backend,
	    const char *version,
	    const char *name,
	    ...);
