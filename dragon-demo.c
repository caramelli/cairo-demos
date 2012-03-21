/*
 * Copyright © 2007 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 *
 * Inspiration (and path!) taken from
 * http://labs.trolltech.com/blogs/2007/08/31/rasterizing-dragons/
 */

#include "demo.h"
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

static int
direction (int i, int np2)
{
	int ret = 1;

	while (i >= 2 && i + 1 < np2) {
		i = np2 - i - 2;
		do
			np2 >>= 1;
		while (2*i < np2);
		ret = !ret;
	}
	return ret;
}

static void
stroke_path (cairo_t *cr, double cx, double cy, int step, int dir, int iterations)
{
	double dx, dy;
	int i, np2;

	switch (dir) {
	default:
	case 0: dx =  step; dy =  0; break;
	case 1: dx = -step; dy =  0; break;
	case 2: dx =  0; dy =  step; break;
	case 3: dx =  0; dy = -step; break;
	}

	np2 = 1;
	cairo_move_to (cr, cx, cy);
	for (i = 0; i < iterations; i++) {
		cairo_rel_line_to (cr, dx, dy);

		if (direction (i, np2)) {
			double t = dx;
			dx = dy;
			dy = -t;
		} else {
			double t = dx;
			dx = -dy;
			dy = t;
		}

		if (i + 1 == np2)
			np2 <<= 1;
	}
	cairo_stroke (cr);
}

static void
fill_path (cairo_t *cr, double x0, double y0, int step, int dir, int iterations)
{
	double dx, dy;
	int i, np2;

	switch (dir) {
	default:
	case 0: dx =  step; dy =  0; break;
	case 1: dx = -step; dy =  0; break;
	case 2: dx =  0; dy =  step; break;
	case 3: dx =  0; dy = -step; break;
	}

	np2 = 1;
	for (i = 0; i < iterations; i++) {
		if (dx) {
			cairo_rectangle (cr, x0, y0-2, dx, 4);
			x0 += dx;
		} else {
			cairo_rectangle (cr, x0-2, y0, 4, dy);
			y0 += dy;
		}
		cairo_fill (cr);

		if (direction (i, np2)) {
			double t = dx;
			dx = dy;
			dy = -t;
		} else {
			double t = dx;
			dx = -dy;
			dy = t;
		}

		if (i + 1 == np2)
			np2 <<= 1;
	}
}

static void
dragon (cairo_t *cr, int width, int height, int iterations, int stroke)
{
	double cx, cy;

	cx = .5 * width;
	cy = .5 * height;

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_paint (cr);

	cairo_set_line_width (cr, 4.);

	cairo_set_source_rgb (cr, 1, 0, 0);
	if (stroke)
		stroke_path (cr, cx, cy, 12, 0, iterations);
	else
		fill_path (cr, cx, cy, 12, 0, iterations);

	cairo_set_source_rgb (cr, 0, 1, 0);
	if (stroke)
		stroke_path (cr, cx, cy, 12, 1, iterations);
	else
		fill_path (cr, cx, cy, 12, 1, iterations);

	cairo_set_source_rgb (cr, 0, 0, 1);
	if (stroke)
		stroke_path (cr, cx, cy, 12, 2, iterations);
	else
		fill_path (cr, cx, cy, 12, 2, iterations);

	cairo_set_source_rgb (cr, 1, 1, 1);
	if (stroke)
		stroke_path (cr, cx, cy, 12, 3, iterations);
	else
		fill_path (cr, cx, cy, 12, 3, iterations);
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
	int iterations = 1;
	int max_iterations;

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int stroke = 1;
	int benchmark;
	const char *version;
	enum clip clip;

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
		if (strcmp (argv[n], "--fill") == 0)
			stroke = 0;
	}

	max_iterations = (device->width + device->height) / 4 / 8;
	max_iterations *= max_iterations;
	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		dragon(cr, device->width, device->height, iterations, stroke);

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
				printf("dragon: %.2f fps\n", frame / delta);
				break;
			}
		}

		if (++iterations == max_iterations)
			iterations = 1;
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		dragon(cr, device->width, device->height, iterations, stroke);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
