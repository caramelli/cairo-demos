#include <cairo.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "list.h"

#define LOREM "Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?"

struct waterfall {
	struct device *device;
	struct list text;
	enum clip clip;
	int min_size, max_size;
};

struct text {
	struct list list;
	double rgba[4];
	int size;
	int y;
	int hue;
};

static void hue_to_rgba (int hue, double *rgba)
{
	double h, s = 1, v = .85;
	double m, n, f;
	int i;

	h = fmod (hue / 16., 6.);
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

static void waterfall_draw(struct waterfall *w, cairo_t *cr)
{
	struct text *t;

	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_paint (cr);

	device_apply_clip(w->device, cr, w->clip);

	list_for_each_entry(t, &w->text, list) {
		cairo_move_to (cr, 0, t->y);
		cairo_set_font_size (cr, t->size);
		cairo_set_source_rgba (cr, t->rgba[0], t->rgba[1], t->rgba[2], t->rgba[3]);
		cairo_show_text (cr, LOREM);
		--t->y;
	}

	if (list_is_empty(&w->text)) {
		t = malloc(sizeof (*t));
		list_add_tail(&t->list, &w->text);
		t->y = w->device->height;
		t->size = w->min_size;
		t->hue = 0;
		hue_to_rgba(t->hue, t->rgba);
	} else {
		t = list_first_entry(&w->text, struct text, list);
		if (t->y + t->size < 0) {
			list_del(&t->list);
			free (t);
		}

		t = list_last_entry(&w->text, struct text, list);
		if (t->y < w->device->height) {
			struct text *tt = malloc(sizeof (*t));
			list_add_tail(&tt->list, &w->text);
			tt->y = t->y + t->size + 2;
			if (t->size < w->device->height)
				tt->size = t->size + 1;
			else
				tt->size = 4;
			if (tt->size < w->min_size)
				tt->size = w->min_size;
			if (tt->size > w->max_size)
				tt->size = w->min_size;
			tt->hue = t->hue + 1;
			hue_to_rgba(tt->hue, tt->rgba);
		}
	}

	cairo_reset_clip (cr);
}

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main (int argc, char **argv)
{
	struct waterfall waterfall;
	struct timeval start, last_tty, last_fps, now;

	double delta;
	int frame = 0;
	int frames = 0;
	int show_fps = 1;
	int benchmark;
	const char *version;

	int n;

	list_init(&waterfall.text);

	waterfall.min_size = 6;
	waterfall.max_size = 96;
	waterfall.device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	waterfall.clip = device_get_clip(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;
	if (benchmark > 0)
		show_fps = 0;

	signal(SIGHUP, signal_handler);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--hide-fps") == 0)
			show_fps = 0;
		if (strncmp (argv[n], "--size=", 7) == 0) {
			int min, max;
			if (sscanf(argv[n]+7, "%d,%d", &min, &max) != 2)
				max = min = atoi(argv[n]+7);
			if (min != 0 && max >= min)
				waterfall.min_size = min, waterfall.max_size = max;
		}
	}

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = waterfall.device->get_framebuffer (waterfall.device);
		cairo_t *cr = cairo_create(fb->surface);

		waterfall_draw(&waterfall, cr);

		gettimeofday(&now, NULL);
		if (show_fps && last_fps.tv_sec) {
			fps_draw(cr, waterfall.device->name, version, &last_fps, &now);
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
				printf("waterfall: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = waterfall.device->get_framebuffer (waterfall.device);
		cairo_t *cr = cairo_create(fb->surface);

		waterfall_draw(&waterfall, cr);
		cairo_destroy(cr);

		fps_finish(fb, waterfall.device->name, version, "waterfall");
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
