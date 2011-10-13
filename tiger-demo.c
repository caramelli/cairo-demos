/*
 * Copyright (c) 2011 Øyvind Kolås <pippin@gimp.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cairo.h>
#include <sys/time.h>

#include "demo.h"

#include "tiger.inc"

static void tiger(struct device *device,
		  struct framebuffer *fb,
		  cairo_antialias_t antialias,
		  enum clip c,
		  double scale,
		  double rotation)
{
	cairo_t *cr;
	int i;

	cr = cairo_create (fb->surface);

	cairo_set_antialias (cr, antialias);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba (cr, 0.1, 0.2, 0.3, 1.0);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	device_apply_clip (device, cr, c);

	cairo_translate (cr, device->width/2, device->height/2);
	cairo_translate (cr, 32, 30);
	cairo_scale (cr, scale, scale);
	cairo_rotate (cr, rotation);
	cairo_translate (cr, -32,-30);

	for (i = 0; i < sizeof (tiger_commands)/sizeof(tiger_commands[0]);i++) {
		DrawCommand *cmd = &tiger_commands[i];
		switch (cmd->type) {
		case 'm':
			cairo_move_to (cr, cmd->x0, cmd->y0);
			break;
		case 'l':
			cairo_line_to (cr, cmd->x0, cmd->y0);
			break;
		case 'c':
			cairo_curve_to (cr,
					cmd->x0, cmd->y0,
					cmd->x1, cmd->y1,
					cmd->x2, cmd->y2);
			break;
		case 'f':
			cairo_set_source_rgba (cr,
					       cmd->x0, cmd->y0, cmd->x1, cmd->y1);
			cairo_fill (cr);
			break;
		}
	}

	cairo_destroy (cr);
}

int main (int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;

	cairo_antialias_t antialias;
	enum clip clip;

	double scale = 0.1;
	double factor = 1.05;
	double rotation = 0.0;

	double delta;
	int frame = 0;
	int frames = 0;
	int benchmark;

	device = device_open(argc, argv);
	antialias = device_get_antialias(argc, argv);
	clip = device_get_clip(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);

		tiger (device, fb, antialias, clip, scale, rotation);

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec) {
			cairo_t *cr = cairo_create (fb->surface);
			fps_draw(cr, device->name, &last_fps, &now);
			cairo_destroy (cr);
		}
		last_fps = now;

		fb->show (fb);
		fb->destroy (fb);

		++frame;
		if (frame % 256 == 0)
			factor = 1./factor;

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

		if (benchmark > 0) {
			delta = now.tv_sec - start.tv_sec;
			delta += (now.tv_usec - start.tv_usec)*1e-6;
			if (delta > benchmark) {
				printf("%.2f fps\n", frame / delta);
				break;
			}
		}

		scale *= factor;
		rotation += 0.1;
	} while (1);

	return 0;
}
