#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"

struct chart {
	int8_t *array;
	int size, pos, scale, y;
	const double *rgba, *rgb;
};

static double rgb[5][3] = {
	{1., 0., 1.},
	{1., 0., 0.},
	{1., 0x66/255., 0.},
	{0., 0., 1.},
	{0., 1., 0.},
};

static const double rgba[5][4] = {
	{1., 176./255, 1., .6},
	{1., 176./255, 176./255, .6},
	{1., 216./255, 176./255, .6},
	{176./255, 176./255, 1., .6},
	{176./255, 1., 176./255, .6},
};

static void chart_init(struct chart *c,
		       int y,
		       int scale,
		       int size,
		       const double rgb[3],
		       const double rgba[4])
{
	int n;

	c->array = malloc(size);
	c->y = y;
	c->scale = scale;
	c->size = size;
	c->pos = 0;

	for (n = 0; n < size; n++)
		c->array[n] = random();

	c->rgb = rgb;
	c->rgba = rgba;
}

static double tangent_at(struct chart *c, int n)
{
	int v[3];

	if (n < 0)
		return c->array[c->pos];
	if (n >= c->size)
		return c->array[(c->pos-1)%c->size];

	v[1] = c->array[(c->pos+n)%c->size];
	if (n > 0)
		v[0] = c->array[(c->pos+n-1)%c->size];
	else
		v[0] = v[1];
	if (n < c->size-1)
		v[2] = c->array[(c->pos+n+1)%c->size];
	else
		v[2] = v[1];

	return (v[2] - v[0])/2. + v[1];
}

static void chart_fill(struct device *device,
		       cairo_t *cr,
		       struct chart *top,
		       struct chart *bottom)
{
	int n;

	cairo_new_path (cr);

	cairo_save (cr);
	cairo_translate (cr, 0, top->y);
	cairo_scale (cr,
		     device->width / (top->size - 1.),
		     top->scale / 128.);
	for (n = 0; n < top->size; n++){
		cairo_curve_to(cr,
			       n - 1./3, tangent_at(top, n - 1),
			       n - 2./3, tangent_at(top, n),
			       n, top->array[(top->pos+n)%top->size]);
	}
	cairo_restore (cr);

	if (bottom) {
		cairo_save (cr);
		cairo_translate (cr, 0, bottom->y);
		cairo_scale (cr,
			     device->width / (bottom->size - 1.),
			     bottom->scale / 128.);
		for (n = bottom->size; n--; )
			cairo_curve_to(cr,
				       n + 2./3, tangent_at(bottom, n + 1),
				       n + 1./3, tangent_at(bottom, n),
				       n, bottom->array[(bottom->pos+n)%bottom->size]);
		cairo_restore (cr);
	} else {
		cairo_line_to(cr, device->width, device->height);
		cairo_line_to(cr, 0, device->height);
	}
	cairo_close_path (cr);

	cairo_set_source_rgba(cr,
			      top->rgba[0], top->rgba[1], top->rgba[2], top->rgba[3]);
	cairo_fill(cr);
}

static void chart_stroke(struct device *device,
			 cairo_t *cr,
			 struct chart *c)
{
	int n;

	cairo_new_path (cr);

	cairo_save (cr);
	cairo_translate (cr, 0, c->y);
	cairo_scale (cr,
		     device->width / (c->size - 1.),
		     c->scale / 128.);
	for (n = 0; n < c->size; n++){
		cairo_curve_to(cr,
			       n - 1./3, tangent_at(c, n - 1),
			       n - 2./3, tangent_at(c, n),
			       n, c->array[(c->pos+n)%c->size]);
	}
	cairo_restore (cr);

	cairo_set_source_rgb(cr, c->rgb[0], c->rgb[1], c->rgb[2]);
	cairo_stroke(cr);
}

static void chart_update(struct chart *c)
{
	c->array[c->pos] = random();
	c->pos = (c->pos + 1) % c->size;
}

static void
bg_draw (struct device *device, cairo_t *cr)
{
	int x, y;

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);

	for (x = 50; x < device->width; x += 50) {
		cairo_move_to (cr, x+.5, 0);
		cairo_line_to (cr, x+.5, device->height);
	}

	for (y = 50; y < device->height; y += 50) {
		cairo_move_to (cr, 0, y+.5);
		cairo_line_to (cr, device->width, y+.5);
	}

	cairo_set_line_width (cr, 1.);
	cairo_set_source_rgb (cr, 0.6, 0.6, 0.6);
	cairo_stroke (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
}

int main (int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int benchmark;
	cairo_antialias_t antialias;
	enum clip clip;

	struct chart c[5];
	int n;

	device = device_open(argc, argv);
	antialias = device_get_antialias(argc, argv);
	clip = device_get_clip(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;
	if (benchmark > 0)
		show_fps = 0;

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--hide-fps") == 0)
			show_fps = 0;
	}

	for (n = 0; n < 5; n++)
		chart_init(&c[n], (n+1)*device->height/6, device->height/15, 100,
			   rgb[n], rgba[n]);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		bg_draw(device, cr);
		device_apply_clip(device, cr, clip);

		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		for (n = 0; n < 5; n++)
			chart_fill(device, cr, &c[n], n + 1 < 5 ? &c[n+1] : NULL);
		cairo_set_line_width (cr, 3);
		cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
		cairo_set_antialias (cr, antialias);

		for (n = 0; n < 5; n++)
			chart_stroke(device, cr, &c[n]);

		gettimeofday(&now, NULL);
		if (show_fps && last_fps.tv_sec) {
			cairo_reset_clip (cr);
			fps_draw(cr, device->name, &last_fps, &now);
		}
		last_fps = now;

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		for (n = 0; n < 5; n++)
			chart_update(&c[n]);

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
				printf("chart: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (1);

	return 0;
}
