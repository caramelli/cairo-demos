#include "demo.h"
#include <string.h>
#include <sys/time.h>
#include <math.h>

static const double m_1_sqrt_3 = 0.577359269;

static void
T (cairo_t *cr, int size, int iterations)
{
	if (iterations-- == 0)
		return;

	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, size, 0);
	cairo_line_to (cr, size/2, size*m_1_sqrt_3);
	cairo_close_path (cr);

	size /= 2;
	if (size >= 4) {
		T (cr, size, iterations);
		cairo_save (cr); {
			cairo_translate (cr, size, 0);
			T (cr, size, iterations);
		} cairo_restore (cr);
		cairo_save (cr); {
			cairo_translate (cr, size/2, size*m_1_sqrt_3);
			T (cr, size, iterations);
		} cairo_restore (cr);
	}
}

static void
draw (cairo_t *cr, int width, int height, int iterations, double theta)
{
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);

    cairo_save (cr);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width (cr, 1.);

    cairo_translate (cr, width/2, height/2);
    cairo_rotate (cr, theta);
    cairo_translate (cr, -width/2, -height/2);

    T (cr, width, iterations);
    cairo_stroke (cr);

    cairo_restore (cr);
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
	int iterations = 1, step = 1, tick = 10;

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int benchmark;
	const char *version;
	enum clip clip;
	double theta = 0;

	int n;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	clip = device_get_clip(argc, argv);
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

		draw(cr, device->width, device->height, iterations, theta);
		if (--tick == 0){
			iterations += step;
			tick = 10;
			if (iterations < 1) {
				iterations = 1;
				step = -step;
			} else if (iterations > log2(device->width)) {
				iterations = log(device->width);
				step = -step;
			}
		}
		theta += 0.1 / 180. * M_PI;

		gettimeofday(&now, NULL);
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
				printf("sierpinski: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		draw(cr, device->width, device->height, 2048, 0);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
