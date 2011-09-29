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
#include <string.h>
#include <math.h>

#include "demo.h"

#define NUMPTS 6

static double animpts[NUMPTS * 2];
static double deltas[NUMPTS * 2];

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

static void
gears_setup(cairo_surface_t *target, int w, int h)
{
	int i;

	//cairo_scale (cr, 3.0, 1.0);

	for (i = 0; i < (NUMPTS * 2); i += 2) {
		animpts[i + 0] = (float) (drand48 () * w);
		animpts[i + 1] = (float) (drand48 () * h);
		deltas[i + 0] = (float) (drand48 () * 6.0 + 4.0);
		deltas[i + 1] = (float) (drand48 () * 6.0 + 4.0);

		if (animpts[i + 0] > w / 2.0)
			deltas[i + 0] = -deltas[i + 0];
		if (animpts[i + 1] > h / 2.0)
			deltas[i + 1] = -deltas[i + 1];
	}
}

static void
stroke_and_fill_animate (double *pts,
			 double *deltas,
			 int index,
			 int limit)
{
	double newpt = pts[index] + deltas[index];

	if (newpt <= 0) {
		newpt = -newpt;
		deltas[index] = (double) (drand48 () * 4.0 + 2.0);
	} else if (newpt >= (double) limit) {
		newpt = 2.0 * limit - newpt;
		deltas[index] = - (double) (drand48 () * 4.0 + 2.0);
	}
	pts[index] = newpt;
}

static void
stroke_and_fill_step (int w, int h)
{
	int i;

	for (i = 0; i < (NUMPTS * 2); i += 2) {
		stroke_and_fill_animate (animpts, deltas, i + 0, w);
		stroke_and_fill_animate (animpts, deltas, i + 1, h);
	}
}

static double gear1_rotation = 0.35;
static double gear2_rotation = 0.33;
static double gear3_rotation = 0.50;

static void
gears_render (cairo_t *cr, int w, int h)
{
	double *ctrlpts = animpts;
	int len = (NUMPTS * 2);
	double prevx = ctrlpts[len - 2];
	double prevy = ctrlpts[len - 1];
	double curx = ctrlpts[0];
	double cury = ctrlpts[1];
	double midx = (curx + prevx) / 2.0;
	double midy = (cury + prevy) / 2.0;
	int i;

	cairo_set_source_rgba (cr, 0.75, 0.75, 0.75, 1.0);
	cairo_set_line_width (cr, 1.0);

	cairo_save (cr);
	cairo_scale (cr, (double) w / 512.0, (double) h / 512.0);

	cairo_save (cr);
	cairo_translate (cr, 170.0, 330.0);
	cairo_rotate (cr, gear1_rotation);
	gear (cr, 30.0, 120.0, 20, 20.0);
	cairo_set_source_rgb (cr, 0.75, 0.45, 0.45);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.25, 0.15, 0.15);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_stroke (cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, 369.0, 330.0);
	cairo_rotate (cr, gear2_rotation);
	gear (cr, 15.0, 75.0, 12, 20.0);
	cairo_set_source_rgb (cr, 0.45, 0.75, 0.45);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.15, 0.25, 0.15);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_stroke (cr);
	cairo_restore (cr);

	cairo_save (cr);
	cairo_translate (cr, 170.0, 116.0);
	cairo_rotate (cr, gear3_rotation);
	gear (cr, 20.0, 90.0, 14, 20.0);
	cairo_set_source_rgb (cr, 0.45, 0.45, 0.75);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve (cr);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_set_source_rgb (cr, 0.15, 0.15, 0.25);
	cairo_stroke (cr);
	cairo_restore (cr);

	cairo_restore (cr);

	gear1_rotation += 0.01;
	gear2_rotation -= (0.01 * (20.0 / 12.0));
	gear3_rotation -= (0.01 * (20.0 / 14.0));

	stroke_and_fill_step (w, h);

	cairo_new_path (cr);
	cairo_move_to (cr, midx, midy);

	for (i = 2; i <= (NUMPTS * 2); i += 2) {
		double x2, x1 = (midx + curx) / 2.0;
		double y2, y1 = (midy + cury) / 2.0;

		prevx = curx;
		prevy = cury;
		if (i < (NUMPTS * 2)) {
			curx = ctrlpts[i + 0];
			cury = ctrlpts[i + 1];
		} else {
			curx = ctrlpts[0];
			cury = ctrlpts[1];
		}
		midx = (curx + prevx) / 2.0;
		midy = (cury + prevy) / 2.0;
		x2 = (prevx + midx) / 2.0;
		y2 = (prevy + midy) / 2.0;
		cairo_curve_to (cr, x1, y1, x2, y2, midx, midy);
	}
	cairo_close_path (cr);

	if (1) {
		double x1, y1, x2, y2;
		cairo_pattern_t *pattern;

		cairo_fill_extents (cr, &x1, &y1, &x2, &y2);

		pattern = cairo_pattern_create_linear (x1, y1, x2, y2);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, 1.0, 0.0, 0.0, 0.6);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.0, 0.0, 1.0, 0.3);
		cairo_pattern_set_filter (pattern, CAIRO_FILTER_BILINEAR);

		cairo_move_to (cr, 0, 0);
		cairo_set_source (cr, pattern);
		cairo_pattern_destroy (pattern);
	}

	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, .1, .75, .1, .9);
	cairo_set_line_width (cr, 3.);
	cairo_stroke (cr);
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

	double delta;
	int frame = 0;
	int frames = 0;
	int benchmark;

	device = device_open(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	gears_setup(device->scanout, device->width, device->height);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		gears_render(cr, device->width, device->height);

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
