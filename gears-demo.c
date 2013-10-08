/*
 * Copyright © 2004 David Reveman, Peter Nilsson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
 * David Reveman and Peter Nilsson not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission. David Reveman and Peter Nilsson
 * makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *
 * DAVID REVEMAN AND PETER NILSSON DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL DAVID REVEMAN AND
 * PETER NILSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: David Reveman <c99drn@cs.umu.se>
 *          Peter Nilsson <c99pnn@cs.umu.se>
 */

#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "demo.h"

static void
gear(cairo_t *cr,
     double inner_radius,
     double outer_radius,
     int teeth,
     double tooth_depth)
{
	int i;
	double r0, r1, r2;
	double angle, da;

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;

	da = 2.0 * M_PI / (double) teeth / 4.0;

	cairo_new_path (cr);

	angle = 0.0;
	cairo_move_to (cr, r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da));

	for (i = 1; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / (double) teeth;

		cairo_line_to (cr, r1 * cos (angle), r1 * sin (angle));
		cairo_line_to (cr, r2 * cos (angle + da), r2 * sin (angle + da));
		cairo_line_to (cr, r2 * cos (angle + 2 * da), r2 * sin (angle + 2 * da));

		if (i < teeth)
			cairo_line_to (cr,
				       r1 * cos (angle + 3 * da),
				       r1 * sin (angle + 3 * da));
	}

	cairo_close_path (cr);

	cairo_new_sub_path(cr);
	cairo_arc_negative(cr, 0, 0, r0, 2*M_PI, 0);
}

static double gear1_rotation = 0.35;
static double gear2_rotation = 0.33;
static double gear3_rotation = 0.50;

static cairo_pattern_t *gear1_sprite;
static cairo_pattern_t *gear2_sprite;
static cairo_pattern_t *gear3_sprite;

static void
gears_render (cairo_t *cr, int w, int h, cairo_antialias_t antialias)
{
	cairo_set_source_rgba (cr, 0.75, 0.75, 0.75, 1.0);
	cairo_set_line_width (cr, 1.0);

	cairo_save (cr);
	cairo_scale (cr, (double) w / 512.0, (double) h / 512.0);

	cairo_save (cr);
	cairo_translate (cr, 170.0, 330.0);
	cairo_rotate (cr, gear1_rotation);
	if (gear1_sprite) {
		cairo_translate (cr, -140, -140);
		cairo_set_source (cr, gear1_sprite);
		cairo_paint (cr);
	} else {
		gear (cr, 30.0, 120.0, 20, 20.0);
		cairo_set_source_rgb (cr, 0.75, 0.45, 0.45);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		cairo_fill_preserve (cr);
		cairo_set_source_rgb (cr, 0.25, 0.15, 0.15);
		cairo_set_antialias (cr, antialias);
		cairo_stroke (cr);
	}
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, 369.0, 330.0);
	cairo_rotate (cr, gear2_rotation);
	if (gear2_sprite) {
		cairo_translate (cr, -95, -95);
		cairo_set_source (cr, gear2_sprite);
		cairo_paint (cr);
	} else {
		gear (cr, 15.0, 75.0, 12, 20.0);
		cairo_set_source_rgb (cr, 0.45, 0.75, 0.45);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		cairo_fill_preserve (cr);
		cairo_set_source_rgb (cr, 0.15, 0.25, 0.15);
		cairo_set_antialias (cr, antialias);
		cairo_stroke (cr);
	}
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, 170.0, 116.0);
	cairo_rotate (cr, gear3_rotation);
	if (gear3_sprite) {
		cairo_translate (cr, -100, -100);
		cairo_set_source (cr, gear3_sprite);
		cairo_paint (cr);
	} else {
		gear (cr, 20.0, 90.0, 14, 20.0);
		cairo_set_source_rgb (cr, 0.45, 0.45, 0.75);
		cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
		cairo_fill_preserve (cr);
		cairo_set_antialias (cr, antialias);
		cairo_set_source_rgb (cr, 0.15, 0.15, 0.25);
		cairo_stroke (cr);
	}
	cairo_restore (cr);

	cairo_restore (cr);

	gear1_rotation += 0.01;
	gear2_rotation -= (0.01 * (20.0 / 12.0));
	gear3_rotation -= (0.01 * (20.0 / 14.0));
}

static void create_sprites (struct device *device, cairo_antialias_t antialias)
{
	struct framebuffer *fb = device->get_framebuffer (device);
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_surface_create_similar (fb->surface,
						CAIRO_CONTENT_COLOR_ALPHA,
						280, 280);
	cr = cairo_create (surface);
	cairo_translate (cr, 140, 140);
	gear (cr, 30.0, 120.0, 20, 20.0);
	cairo_set_source_rgb (cr, 0.75, 0.45, 0.45);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.25, 0.15, 0.15);
	cairo_set_antialias (cr, antialias);
	cairo_stroke (cr);
	cairo_destroy (cr);
	gear1_sprite = cairo_pattern_create_for_surface (surface);
	cairo_surface_destroy (surface);

	surface = cairo_surface_create_similar (fb->surface,
						CAIRO_CONTENT_COLOR_ALPHA,
						190, 190);
	cr = cairo_create (surface);
	cairo_translate (cr, 95, 95);
	gear (cr, 15.0, 75.0, 12, 20.0);
	cairo_set_source_rgb (cr, 0.45, 0.75, 0.45);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.15, 0.25, 0.15);
	cairo_set_antialias (cr, antialias);
	cairo_stroke (cr);
	cairo_destroy (cr);
	gear2_sprite = cairo_pattern_create_for_surface (surface);
	cairo_surface_destroy (surface);

	surface = cairo_surface_create_similar (fb->surface,
						CAIRO_CONTENT_COLOR_ALPHA,
						200, 200);
	cr = cairo_create (surface);
	cairo_translate (cr, 100, 100);
	gear (cr, 20.0, 90.0, 14, 20.0);
	cairo_set_source_rgb (cr, 0.45, 0.45, 0.75);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_antialias (cr, antialias);
	cairo_set_source_rgb (cr, 0.15, 0.15, 0.25);
	cairo_stroke (cr);
	cairo_destroy (cr);
	gear3_sprite = cairo_pattern_create_for_surface (surface);
	cairo_surface_destroy (surface);
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
	const char *version = NULL;
	int benchmark;
	int show_fps = 1;
	int use_sprites = 0;
	cairo_antialias_t antialias;
	int n;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;
	if (benchmark > 0)
		show_fps = 0;
	antialias = device_get_antialias(argc, argv);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--hide-fps") == 0)
			show_fps = 0;
		if (strcmp (argv[n], "--use-sprites") == 0)
			use_sprites = 1;
	}

	if (use_sprites)
		create_sprites (device, antialias);

	signal(SIGHUP, signal_handler);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		gears_render(cr, device->width, device->height, antialias);

		gettimeofday(&now, NULL);
		if (show_fps && last_fps.tv_sec)
			fps_draw(cr, device->name, version, &last_fps, &now);
		last_fps = now;

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		frame++;
		if (benchmark > 0) {
			delta = now.tv_sec - start.tv_sec;
			delta += (now.tv_usec - start.tv_usec)*1e-6;
			if (delta > benchmark) {
				printf("gears: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		gears_render(cr, device->width, device->height, antialias);

		cairo_destroy(cr);

		fps_finish(fb, device->name, version, "gears");
		fb->show (fb);
		fb->destroy (fb);
		pause ();
	}

	return 0;
}
