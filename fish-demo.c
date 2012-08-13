#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "demo.h"

static int fish_width, fish_height;

struct fish {
	int x, y, z;
	int dx, dy, dz;
	int frame, rgb;
};

static cairo_pattern_t *solid_background(void)
{
	return cairo_pattern_create_rgb (0.2, 0.4, 0.6);
}

static cairo_pattern_t *create_background(struct device *device, int *x1, int *x2)
{
	cairo_surface_t *surface, *image;
	cairo_pattern_t *pattern;
	cairo_matrix_t m;
	int width, height;
	double sf;
	cairo_t *cr;

	image = surface_create_from_png(device->scanout, "fishbg.png");
	height = cairo_image_surface_get_height(image);
	width = cairo_image_surface_get_width(image);
	sf = height / (double)device->height;

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

	*x1 = (device->width - width / sf) / 2;
	*x2 = device->width - *x1;

	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);
	cairo_matrix_init_scale(&m, sf,sf);
	cairo_matrix_translate(&m, -*x1, 0);
	cairo_pattern_set_matrix(pattern, &m);

	return pattern;
}

static cairo_surface_t **create_strip(struct device *device)
{
	cairo_surface_t *surface, *image, **strip;
	int width, height, x, y;
	cairo_t *cr;

	image = surface_create_from_png(device->scanout, "fishstrip.png");
	width = cairo_image_surface_get_width(image);
	height = cairo_image_surface_get_height(image);
	surface = cairo_surface_create_similar(device->scanout,
					       cairo_surface_get_content (image),
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

	strip = (cairo_surface_t **)malloc(sizeof(cairo_surface_t*)*48);
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

static void fish_init(struct device *device, struct fish *f, int x1, int x2)
{
	int w = x2 - x1;

	f->x = random() % (w - fish_width) + x1;
	f->y = random() % (device->height - fish_height);
	f->z = 0;

	f->dx = random() % 5 + 1;
	if (random() % 2)
		f->dx = -f->dx;
	f->dy = random() % 3;
	if (random() % 2)
		f->dy = -f->dy;
	f->dz = 0;

	f->frame = random() % 16;
	f->rgb = random() % 3;
}

static void fish_draw(struct device *device, cairo_t *cr, struct fish *f, cairo_pattern_t *reflection, int x1, int x2, cairo_surface_t **strip)
{
	double sx = f->dx > 0 ? 1 : -1;
	double tx = f->dx > 0 ? 0 : -fish_width;

	cairo_save(cr);
	cairo_scale(cr, sx, 1);
	cairo_set_source_surface(cr, strip[f->rgb*16+f->frame], sx*f->x+tx, f->y);
	cairo_paint(cr);
	cairo_restore(cr);

	if (reflection) {
		sx = -sx;
		tx = f->dx > 0 ? -fish_width : 0;
		tx -= sx*fish_width;

		cairo_save(cr);
		cairo_rectangle (cr, 0, 0, x1, device->height);
		cairo_clip (cr);
		cairo_scale(cr, sx, 1);
		cairo_set_source_surface(cr, strip[f->rgb*16+f->frame], sx*(2*x1 - f->x) + tx, f->y);
		cairo_identity_matrix(cr);
		cairo_mask(cr, reflection);
		cairo_restore(cr);

		cairo_save(cr);
		cairo_rectangle (cr, x2, 0, device->width-x2, device->height);
		cairo_clip (cr);
		cairo_scale(cr, sx, 1);
		cairo_set_source_surface(cr, strip[f->rgb*16+f->frame], sx*(2*x2 - f->x) + tx, f->y);
		cairo_identity_matrix(cr);
		cairo_mask(cr, reflection);
		cairo_restore(cr);
	}

	f->frame = (f->frame+1) % 16;
	f->x += f->dx;
	f->y += f->dy;

	if (f->x < x1-10 || f->x > x2-fish_width+10) {
		f->dx = -f->dx;
	}
	if (f->y < -10 || f->y > device->height - fish_height+10)
		f->dy = -f->dy;
}

static int done;
static void signal_handler(int sig)
{
	done = sig;
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
	const char *version;
	enum clip clip;
	cairo_pattern_t *reflection = NULL;
	int solid_bg = 0;
	int x1, x2;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	clip = device_get_clip(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	signal(SIGHUP, signal_handler);

	for (n = 1; n < argc; n++) {
		if (strncmp(argv[n], "--num-fish=", 11) == 0)
			num_fish = atoi(argv[n]+11);
		else if (strcmp(argv[n], "--reflection") == 0)
			reflection = (cairo_pattern_t *)1;
		else if (strcmp(argv[n], "--solid-background") == 0)
			solid_bg = 1;
	}

	x1 = -1;
	if (solid_bg)
		bg = solid_background();
	else
		bg = create_background(device, &x1, &x2);
	if (cairo_pattern_status(bg))
		return 1;

	if (reflection != NULL && x1 >= 0) {
		reflection = cairo_pattern_create_linear(0,0, device->width, 0);
		cairo_pattern_add_color_stop_rgba(reflection, 0, 0, 0, 0, 0);
		cairo_pattern_add_color_stop_rgba(reflection, x1/(double)device->width, 0, 0, 0, .75);
		cairo_pattern_add_color_stop_rgba(reflection, x1/(double)device->width, 0, 0, 0, 1);
		cairo_pattern_add_color_stop_rgba(reflection, x2/(double)device->width, 0, 0, 0, 1);
		cairo_pattern_add_color_stop_rgba(reflection, x2/(double)device->width, 0, 0, 0, .75);
		cairo_pattern_add_color_stop_rgba(reflection, 1, 0, 0, 0, 0);
	} else
		x1 = 0, x2 = device->width;

	strip = create_strip(device);

	fish = (struct fish *)malloc(sizeof(*fish)*num_fish);
	for (n = 0; n < num_fish; n++)
		fish_init(device, &fish[n], x1, x2);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		device_apply_clip(device, cr, clip);

		cairo_set_source(cr, bg);
		if (reflection)
			cairo_mask(cr, reflection);
		else
			cairo_paint(cr);

		for (n = 0; n < num_fish; n++)
			fish_draw(device, cr, &fish[n], reflection, x1, x2, strip);

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec) {
			cairo_reset_clip (cr);
			fps_draw(cr, device->name, version, &last_fps, &now);
		}
		last_fps = now;

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		if (benchmark < 0 && 0) {
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
				printf("fish: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		device_apply_clip(device, cr, clip);

		cairo_set_source(cr, bg);
		if (reflection)
			cairo_mask(cr, reflection);
		else
			cairo_paint(cr);

		for (n = 0; n < num_fish; n++)
			fish_draw(device, cr, &fish[n], reflection, x1, x2, strip);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
