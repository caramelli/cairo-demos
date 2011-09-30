#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include "demo.h"

static int fish_width, fish_height;

struct fish {
	int x, y, z;
	int dx, dy, dz;
	int frame, rgb;
};

static cairo_pattern_t *create_background(struct device *device)
{
	cairo_surface_t *surface, *image;
	cairo_pattern_t *pattern;
	cairo_matrix_t m;
	int width, height;
	double sf;
	cairo_t *cr;

	image = cairo_image_surface_create_from_png("fishbg.png");

	height = cairo_image_surface_get_height(image);
	width = cairo_image_surface_get_width(image);
	sf = height/ (double)device->height;

	surface = cairo_surface_create_similar(device->scanout,
					       CAIRO_CONTENT_COLOR,
					       width, height);
	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_set_source_surface(cr, image, 0, 0);
	cairo_surface_destroy(image);
	cairo_paint(cr);
	pattern = cairo_pattern_create_for_surface(cairo_get_target(cr));
	cairo_destroy (cr);

	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);
	cairo_matrix_init_scale(&m, sf,sf);
	cairo_pattern_set_matrix(pattern, &m);
	return pattern;
}

static cairo_surface_t **create_strip(struct device *device)
{
	cairo_surface_t *surface, *image, **strip;
	int width, height, x, y;
	cairo_t *cr;

	image = cairo_image_surface_create_from_png("fishstrip.png");
	width = cairo_image_surface_get_width(image);
	height = cairo_image_surface_get_height(image);
	surface = cairo_surface_create_similar(device->scanout,
					       CAIRO_CONTENT_COLOR_ALPHA,
					       width, height);
	cr = cairo_create(surface);
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_paint(cr);
	cairo_destroy (cr);

	cairo_surface_destroy(image);

	width /= 16;
	height /= 3;

	fish_width = width;
	fish_height = height;

	strip = malloc(sizeof(cairo_surface_t*)*48);
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 16; x++) {
			strip[y*16+x] =
				cairo_surface_create_for_rectangle(surface,
								   x * width,
								   y * height,
								   width,
								   height);

		}
	}

	return strip;
}

static void fish_init(struct device *device, struct fish *f)
{
	f->x = random() % (device->width + fish_width) - fish_width;
	f->y = random() % (device->height + fish_height) - fish_height;
	f->z = 0;

	f->dx = random() % 10 + 1;
	if (random() % 2)
		f->dx = -f->dx;
	f->dy = random() % 10 + 1;
	if (random() % 2)
		f->dy = -f->dy;
	f->dz = 0;

	f->frame = random() % 16;
	f->rgb = random() % 3;
}

static void fish_draw(struct device *device, cairo_t *cr, struct fish *f, cairo_surface_t **strip)
{
	double sx = f->dx > 0 ? 1 : -1;
	double tx = f->dx > 0 ? 0 : -fish_width;

	cairo_save(cr);
	cairo_scale(cr, sx, 1);
	cairo_set_source_surface(cr, strip[f->rgb*16+f->frame], sx*f->x+tx, f->y);
	cairo_paint(cr);
	cairo_restore(cr);

	f->frame = (f->frame+1) % 16;
	f->x += f->dx;
	f->y += f->dy;

	if (f->x < -fish_width || f->x >= device->width)
		f->dx = -f->dx;
	if (f->y < -fish_height || f->y >= device->height)
		f->dy = -f->dy;
}

static void
fps_draw (struct framebuffer *fb, const char *name,
	  const struct timeval *last,
	  const struct timeval *now)
{
#define N_FILTER 25
    static double filter[25];
    static int filter_pos;
    cairo_text_extents_t extents;
    char buf[80];
    double fps, avg;
    int n, max;
    cairo_t *cr;

    fps = now->tv_sec - last->tv_sec;
    fps += (now->tv_usec - last->tv_usec) / 1000000.;

    max = N_FILTER;
    avg = fps;
    if (filter_pos < max)
	max = filter_pos;
    for (n = 0; n < max; n++)
	avg += filter[n];
    avg /= max + 1;
    filter[filter_pos++ % N_FILTER] = fps;
    if (filter_pos < 5)
	    return;

    cr = cairo_create (fb->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    snprintf (buf, sizeof (buf), "%s: %.1f fps", name, 1. / avg);
    cairo_set_font_size (cr, 18);
    cairo_text_extents (cr, buf, &extents);

    cairo_rectangle (cr, 4-1, 4-1, extents.width+2, extents.height+2);
    cairo_set_source_rgba (cr, .0, .0, .0, .85);
    cairo_fill (cr);

    cairo_move_to (cr, 4 - extents.x_bearing, 4 - extents.y_bearing);
    cairo_set_source_rgb (cr, .95, .95, .95);
    cairo_show_text (cr, buf);

    cairo_destroy (cr);
}

int main (int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;

	cairo_pattern_t *bg;
	cairo_surface_t **strip;
	struct fish *fish;
	int num_fish = 20, n;

	double delta;
	int frame = 0;
	int frames = 0;
	int benchmark;

	device = device_open(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	for (n = 1; n < argc; n++) {
		if (strncmp(argv[n], "--num-fish=", 11) == 0)
			num_fish = atoi(argv[n]+11);
	}

	bg = create_background(device);
	if (cairo_pattern_status(bg))
		return 1;

	fish = malloc(sizeof(*fish)*num_fish);
	for (n = 0; n < num_fish; n++)
		fish_init(device, &fish[n]);

	strip = create_strip(device);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);
		cairo_set_source(cr, bg);
		cairo_paint(cr);

		for (n = 0; n < num_fish; n++)
			fish_draw(device, cr, &fish[n], strip);

		cairo_destroy(cr);

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec)
			fps_draw(fb, device->name, &last_fps, &now);
		last_fps = now;

		fb->show (fb);
		fb->destroy (fb);

		if (benchmark < 0) {
			delta = now.tv_sec - last_tty.tv_sec;
			delta += (now.tv_usec - last_tty.tv_usec)*1e-6;
			frames++;
			if (delta >  5) {
				printf("%.2f fps\n", frames/delta);
				last_tty = now;
				frames = 0;
			}
		}

		frame++;
		if (benchmark > 0) {
			delta = now.tv_sec - start.tv_sec;
			delta += (now.tv_usec - start.tv_usec)*1e-6;
			if (delta > benchmark) {
				printf("%.2f fps\n", frame / delta);
				break;
			}
		}
	} while (1);

	return 0;
}
