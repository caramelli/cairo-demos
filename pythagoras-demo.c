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
 */

#include "demo.h"

#define _USE_MATH_DEFINES /* for M_SQRT2 on win32 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#ifndef MAX
#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#endif

static void
add_rectangle (cairo_t *cr, double size)
{
	double x, y;

	if (size < .5)
		return;

	cairo_get_current_point (cr, &x, &y);

	cairo_rel_move_to (cr, -size/2., -size/2.);
	cairo_rel_line_to (cr, size, 0);
	cairo_rel_line_to (cr, 0, size);
	cairo_rel_line_to (cr, -size, 0);
	cairo_close_path (cr);

	cairo_save (cr);
	cairo_translate (cr, -size/2., size);
	cairo_move_to (cr, x, y);
	cairo_rotate (cr, M_PI/4);
	add_rectangle (cr, size / M_SQRT2);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, size/2., size);
	cairo_move_to (cr, x, y);
	cairo_rotate (cr, -M_PI/4);
	add_rectangle (cr, size / M_SQRT2);
	cairo_restore (cr);
}

static void
pythagoras (cairo_t *cr, int width, int height, double size)
{
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);

	cairo_save (cr);
	cairo_translate (cr, 0, height);
	cairo_scale (cr, 1, -1);

	cairo_move_to (cr, width/2, height/2-2*size);
	add_rectangle (cr, size);
	cairo_set_source_rgb (cr, 0.05, 0.3, 0.05);
	cairo_fill (cr);
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

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int benchmark;
	const char *version;
	enum clip clip;
	double size = 2;

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

		pythagoras(cr, device->width, device->height, size);
		size *= 1.2;
		if (size >= MAX(device->width, device->height))
			size = 2;

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
				printf("pythagoras: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		pythagoras(cr, device->width, device->height, 2048);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
