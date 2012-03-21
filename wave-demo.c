/*
 * Copyright 2011 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Benjamin Otte <otte@redhat.com>
 */

#include "demo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static unsigned int seed = 0xdeadbeef;
static int first;

static cairo_surface_t *
generate_random_waveform(cairo_t *target, int width, int height)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	int i, r;
	unsigned int this_seed = seed;

	surface = cairo_surface_create_similar (cairo_get_target (target),
						CAIRO_CONTENT_ALPHA,
						width, height);
	cr = cairo_create (surface);

	r = first;
	for (i = 0; i < width; i++) {
		r += rand_r (&this_seed) % (height / 4) - (height / 8);
		if (r < 0)
			r = 0;
		else if (r > height - 2)
			r = height - 2;
		cairo_rectangle (cr, i, (height - r) / 2, 1, r);
		cairo_fill (cr);
		if (i == 0) {
			seed = this_seed;
			first = r;
		}
	}
	cairo_destroy (cr);

	return surface;
}

static void
wave (cairo_t *cr, int width, int height)
{
	cairo_surface_t *wave;
	cairo_pattern_t *mask;

	wave = generate_random_waveform (cr, width+2, height+2);

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_paint (cr);

	/* paint outline (and contents) */
	cairo_set_source_rgb (cr, 1, 0, 0);
	cairo_mask_surface (cr, wave, -1, -1);

	/* overdraw inline */
	/* first, create a mask */
	cairo_push_group_with_content (cr, CAIRO_CONTENT_ALPHA);
	cairo_set_source_surface (cr, wave, -1, -1);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_IN);
	cairo_set_source_surface (cr, wave, 0, -1);
	cairo_paint (cr);
	cairo_set_source_surface (cr, wave, -2, -1);
	cairo_paint (cr);
	cairo_set_source_surface (cr, wave, -1, 0);
	cairo_paint (cr);
	cairo_set_source_surface (cr, wave, -1, -2);
	cairo_paint (cr);
	mask = cairo_pop_group (cr);

	/* second, paint the mask */
	cairo_set_source_rgb (cr, 0, 1, 0);
	cairo_mask (cr, mask);

	cairo_pattern_destroy (mask);
	cairo_surface_destroy (wave);
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
	first = device->height / 2;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		wave(cr, device->width, device->height);

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
				printf("wave: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		wave(cr, device->width, device->height);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
