#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"

static int vv;

static void hs_to_rgba (double h, double s, double *rgba)
{
	double v = .85;
	double m, n, f;
	int i;

	v = fmod(vv++ / 2000., 2);
	if (v > 1)
		v = 2 - v;

	h = fmod(h, 2*M_PI);
	h *= 6 / (2 * M_PI);
	i = floor (h);
	f = h - i;
	if ((i & 1) == 0)
		f = 1 - f;

	m = v * (1 - s);
	n = v * (1 - s * f);
	switch(i){
	case 6:
	case 0: rgba[0] = v; rgba[1] = n; rgba[2] = m; break;
	case 1: rgba[0] = n; rgba[1] = v; rgba[2] = m; break;
	case 2: rgba[0] = m; rgba[1] = v; rgba[2] = n; break;
	case 3: rgba[0] = m; rgba[1] = n; rgba[2] = v; break;
	case 4: rgba[0] = n; rgba[1] = m; rgba[2] = v; break;
	case 5: rgba[0] = v; rgba[1] = m; rgba[2] = n; break;
	}
	rgba[3] = 1.;
}

static void spiral_draw(struct device *device, cairo_t *cr,
			cairo_antialias_t antialias, enum clip clip)
{
	double r, t, max;
	double mx, my;

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_paint (cr);

	cairo_save (cr);
	device_apply_clip(device, cr, clip);
	cairo_set_antialias (cr, antialias);

	mx = device->width/2;
	my = device->height/2;
	max = mx*mx + my*my;

	r = t = 0;
	while (r*r < max) {
		double x0, y0, x1, y1, rgba[4];
		double dx0, dy0, dx1, dy1;

		x0 = mx + r*cos(t);
		y0 = my + r*sin(t);
		dx0 = -r*sin(t);
		dy0 = r*cos(t);
		cairo_move_to (cr, x0, y0);

		r += 0.2;
		t += 0.1;

		x1 = mx + r*cos(t);
		y1 = my + r*sin(t);
		dx1 = -r*sin(t);
		dy1 = r*cos(t);
		cairo_curve_to (cr,
				x0+dx0/100, y0+dy0/100,
				x1-dx1/100, y1-dy1/100,
				x1, y1);

		hs_to_rgba(t, r*r/max, rgba);
		cairo_set_source_rgba (cr, rgba[0], rgba[1], rgba[2], rgba[3]);
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main (int argc, char **argv)
{
	struct timeval start, last_tty, last_fps, now;
	struct device *device;
	enum clip clip;

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int benchmark;
	cairo_antialias_t antialias;
	const char *version;

	int n;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	clip = device_get_clip(argc, argv);
	antialias = device_get_antialias(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;
	if (benchmark > 0)
		show_fps = 0;

	signal(SIGHUP, signal_handler);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--hide-fps") == 0)
			show_fps = 0;
	}

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		gettimeofday(&now, NULL);
		vv = 1000*(now.tv_sec-start.tv_sec)+(now.tv_usec-start.tv_usec)/1000;
		spiral_draw(device, cr, antialias, clip);

		if (show_fps && last_fps.tv_sec) {
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
				printf("chart: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		spiral_draw(device, cr, antialias, clip);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
