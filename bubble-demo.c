#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "demo.h"

struct surface {
	cairo_surface_t *surface;
	cairo_pattern_t *pattern;
	int width, height;
	double sf_x, sf_y;
};

struct bubble {
	double x, y;
	double dx, dy;
	double scale, radius;
	struct surface *fg;
};

static int create_background(struct device *device, struct surface *s)
{
	cairo_surface_t *image;
	cairo_matrix_t m;
	cairo_t *cr;

	image = surface_create_from_png(device->scanout, "bubblebg.png");
	s->height = cairo_image_surface_get_height(image);
	s->width = cairo_image_surface_get_width(image);

	s->surface = cairo_surface_create_similar(device->scanout,
						  CAIRO_CONTENT_COLOR,
						  s->width, s->height);
	cr = cairo_create(s->surface);
	cairo_surface_destroy(s->surface);
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_surface_destroy(image);
	cairo_paint(cr);

	s->surface = cairo_surface_reference(cairo_get_target(cr));
	cairo_destroy (cr);

	s->sf_x = s->width / (double)device->width;
	s->sf_y = s->height / (double)device->height;
	cairo_matrix_init_scale(&m, s->sf_x, s->sf_y);
	s->pattern = cairo_pattern_create_for_surface(s->surface);
	cairo_pattern_set_matrix(s->pattern, &m);

	return cairo_pattern_status(s->pattern) == CAIRO_STATUS_SUCCESS;
}

static int create_gauss(struct device *device, struct surface *s, double radius)
{
	cairo_surface_t *image, *surface;
	cairo_t *cr;
	int size, i, j, stride;
	uint8_t *pixel;

	size = 4*ceil(radius);
	size |= 1;

	image = cairo_image_surface_create(CAIRO_FORMAT_A8, size, size);
	if (cairo_surface_status(image))
		return 0;

	pixel = (uint8_t *)cairo_image_surface_get_data(image);
	stride = cairo_image_surface_get_stride(image) / sizeof(uint8_t);
	cairo_surface_flush(image);
	for (j = 0; j < size; j++) {
		double dy = j - size / 2.;
		for (i = 0; i < size; i++) {
			double dx = i - size / 2.;
			pixel[j*stride + i] = 96*exp(-(dx*dx + dy*dy)/(radius*radius));
		}
	}
	cairo_surface_mark_dirty(image);

	surface = cairo_surface_create_similar(device->scanout,
					       CAIRO_CONTENT_ALPHA,
					       size, size);
	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_set_source_surface(cr, image, 0, 0);
	cairo_surface_destroy(image);
	cairo_paint(cr);
	s->surface = cairo_surface_reference(cairo_get_target(cr));
	cairo_destroy (cr);

	s->pattern = cairo_pattern_create_for_surface(s->surface);
	s->height = s->width = size;
	s->sf_x = s->sf_y = 1.;

	return cairo_pattern_status(s->pattern) == CAIRO_STATUS_SUCCESS;
}

static int create_mask(struct device *device, struct surface *s)
{
	s->width = device->width;
	s->height = device->height;
	s->surface = cairo_surface_create_similar(device->scanout,
						  CAIRO_CONTENT_ALPHA,
						  s->width, s->height);
	s->pattern = cairo_pattern_create_for_surface(s->surface);
	s->sf_x = s->sf_y = 1.;

	return cairo_pattern_status(s->pattern) == CAIRO_STATUS_SUCCESS;
}

static int create_foreground(struct device *device, struct surface *s)
{
	cairo_surface_t *image;
	cairo_t *cr;

	image = surface_create_from_png(device->scanout, "bubble.png");
	s->height = cairo_image_surface_get_height(image);
	s->width = cairo_image_surface_get_width(image);

	s->surface = cairo_surface_create_similar(device->scanout,
						  CAIRO_CONTENT_COLOR_ALPHA,
						  s->width, s->height);
	cr = cairo_create(s->surface);
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_surface_destroy(image);
	cairo_paint(cr);
	cairo_destroy(cr);

	s->pattern = NULL;
	s->sf_x = s->sf_y = 1.;
	return 1;
}

static int bubble_overlap(struct bubble *a, struct bubble *b)
{
	double dx = a->x - b->x;
	double dy = a->y - b->y;
	double r = a->radius + b->radius;
	return dx * dx + dy * dy < r * r;
}

static void bubble_init(struct device *device,
			struct bubble *bubbles, int n,
			struct surface *fg, double scale)
{
	struct bubble *b = &bubbles[n];
	int retry = 100;
	int m;

	do {
		b->dx = (random() % 5 + 1) / 3.;
		if (random() % 2)
			b->dx = -b->dx;
		b->dy = (random() % 5 + 1) / 3.;
		if (random() % 2)
			b->dy = -b->dy;

		b->fg = fg;
		b->scale = 100. * scale / (random() % 50 + 50);
		b->radius = hypot(b->fg->width/2, b->fg->height/2) * b->scale / M_SQRT2;
		b->dx /= b->scale / scale;
		b->dy /= b->scale / scale;

		do {
			b->x = random() % device->width;
			b->y = random() % device->height;
		} while (b->x < fg->width * b->scale/2 ||
			 b->x > device->width - fg->width * b->scale/2 ||
			 b->y < fg->height * b->scale/2 ||
			 b->y > device->height - fg->height * b->scale/2);

		for (m = 0; m < n; m++) {
			struct bubble *a = &bubbles[m];
			if (bubble_overlap(a,b))
				break;
		}
	} while (retry-- && m != n);
}

static void bubble_draw(cairo_t *cr, struct bubble *f)
{
	double scale = f->scale;
	cairo_save(cr);
	cairo_scale(cr, scale, scale);
	cairo_set_source_surface(cr, f->fg->surface,
				 (f->x-f->fg->width/2.)/scale,
				 (f->y-f->fg->height/2.)/scale);
	cairo_paint(cr);
	cairo_restore(cr);
}

static void bubble_move(struct device *device, struct bubble *f)
{
	f->x += f->dx;
	f->y += f->dy;

	if (f->x < f->fg->width*f->scale/2 ||
	    f->x > device->width-f->fg->width*f->scale/2) {
		f->dx = -f->dx;
		f->x += f->dx;
	}

	if (f->y < f->fg->height*f->scale/2 ||
	    f->y > device->height-f->fg->height*f->scale/2) {
		f->dy = -f->dy;
		f->y += f->dy;
	}
}

static void bubble_collide(struct bubble *a, struct bubble *b,
			   cairo_t *cr, struct surface *o)
{
	double dx, dy, dvx, dvy, d2, r;

	r = a->radius + b->radius;

	dx = a->x - b->x;
	dy = a->y - b->y;
	d2 = dx * dx + dy * dy;
	if (d2 > r*r)
		return;

	r = (a->scale + b->scale) / 2.;
	cairo_save(cr);
	cairo_scale(cr, r, r);
	cairo_set_source_surface(cr, o->surface,
				 (a->x + b->x - o->width)/(2 * r),
				 (a->y + b->y - o->height)/(2 * r));
	cairo_paint(cr);
	cairo_restore(cr);

	dvx = a->dx - b->dx;
	dvy = a->dy - b->dy;

        d2 = (dvx * dx + dvy * dy) / d2;

	a->dx -= dx * d2;
	b->dx += dx * d2;

	a->dy -= dy * d2;
	b->dy += dy * d2;
}

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main(int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;

	struct surface fg, gauss, mask, bg;
	struct bubble *bubble;
	int num_bubbles = 100, n, m;

	double delta;
	int frame = 0;
	int benchmark;
	const char *version;
	enum clip clip;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	clip = device_get_clip(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	signal(SIGHUP, signal_handler);

	for (n = 1; n < argc; n++) {
		if (strncmp(argv[n], "--num-bubbles=", 14) == 0)
			num_bubbles = atoi(argv[n]+14);
	}

	if (!create_foreground(device, &fg))
		return 1;

	if (!create_background(device, &bg))
		return 1;

	if (!create_mask(device, &mask))
		return 1;

	if (!create_gauss(device, &gauss, hypot(fg.width, fg.height) / 5))
		return 1;

	delta = (device->width * device->height) / (num_bubbles * fg.width * fg.height * 25.);

	bubble = (struct bubble *)malloc(sizeof(*bubble)*num_bubbles);
	for (n = 0; n < num_bubbles; n++)
		bubble_init(device, bubble, n, &fg, delta);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		device_apply_clip(device, cr, clip);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source(cr, bg.pattern);
		cairo_mask(cr, mask.pattern);

		for (n = 0; n < num_bubbles; n++)
			bubble_draw(cr, &bubble[n]);

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec) {
			cairo_reset_clip (cr);
			fps_draw(cr, device->name, version, &last_fps, &now);
		}
		last_fps = now;

		cairo_destroy(cr);

		fb->show(fb);
		fb->destroy(fb);

		for (n = 0; n < num_bubbles; n++)
			bubble_move(device, &bubble[n]);

		cr = cairo_create(mask.surface);
		cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
		for (n = 0; n < num_bubbles; n++)
			for (m = n + 1; m < num_bubbles; m++)
				bubble_collide(&bubble[n], &bubble[m],
					       cr, &gauss);
		cairo_destroy(cr);

		frame++;
		if (benchmark > 0) {
			delta = now.tv_sec - start.tv_sec;
			delta += (now.tv_usec - start.tv_usec)*1e-6;
			if (delta > benchmark) {
				printf("bubble: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		device_apply_clip(device, cr, clip);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source(cr, bg.pattern);
		cairo_mask(cr, mask.pattern);

		for (n = 0; n < num_bubbles; n++)
			bubble_draw(cr, &bubble[n]);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version, "bubble");
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
