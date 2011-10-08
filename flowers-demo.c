/*
 * Pretty cairo flower hack.
 *
 * Removed the redundant clutter.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <sys/time.h>

#include "demo.h"

#define PETAL_MIN 20
#define PETAL_VAR 40
#define N_FLOWERS 40

#define ARRAY_LENGTH(A) ((int) sizeof (A) / sizeof (A[0]))

typedef struct _flower {
    cairo_pattern_t *pattern;
    int n_groups;
    int n_petals[4];
    int color[5];
    int rotation[4];
    int pm1[4], pm2[4];
    int size, petal_size[5];
    int x, y, v;
    double rot, rv;
    double fade, dfade;
} flower_t;

static flower_t flowers[N_FLOWERS];

/* No science here, just a hack from toying */
static const struct {
    double red, green, blue;
} colors[] = {
    { 0.71, 0.81, 0.83 },
    { 1.00, 0.78, 0.57 },
    { 0.64, 0.30, 0.35 },
    { 0.73, 0.40, 0.39 },
    { 0.91, 0.56, 0.64 },
    { 0.70, 0.47, 0.45 },
    { 0.92, 0.75, 0.60 },
    { 0.82, 0.86, 0.85 },
    { 0.51, 0.56, 0.67 },
    { 1.00, 0.79, 0.58 },
};

static void
make_fast_flower (cairo_surface_t *target, flower_t *f, int w, int h)
{
    int petal_size;
    int n_groups;    /* Num groups of petals 1-3 */
    int n_petals;    /* num of petals 4 - 8  */
    int pm1, pm2;
    int idx, last_idx = -1;
    int i, j;

    cairo_surface_t *surface;
    cairo_t *cr;

    n_groups = rand () % 3 + 1;
    petal_size = PETAL_MIN + rand () % PETAL_VAR;
    f->size = petal_size * 8;

    f->x = (rand() % w - f->size) + f->size/2.;
    f->y = rand() % h;
    f->rot = fmod (rand(), 2 * M_PI);
    f->rv = (fmod (rand(), 5.) + 1) * M_PI * 2. /360.;
    f->v  = rand() % 10 + 2;
    f->fade = fmod (rand(), 100) / 100.;
    f->dfade = .01 * fmod (rand(), 100) / 100.;

    surface = cairo_surface_create_similar (target,
					    CAIRO_CONTENT_COLOR_ALPHA,
					    f->size, f->size);
    cr = cairo_create (surface);
    cairo_surface_destroy (surface);

    cairo_translate (cr, f->size/2., f->size/2.);

    for (i = 0; i < n_groups; i++) {
	n_petals = rand () % 5 + 4;
	cairo_save (cr);

	cairo_rotate (cr, rand() % 6);

	do {
	    idx = rand () % ARRAY_LENGTH (colors);
	} while (idx == last_idx);

	cairo_set_source_rgba (cr,
			       colors[idx].red,
			       colors[idx].green,
			       colors[idx].blue,
			       0.5);

	last_idx = idx;

	/* some bezier randomness */
	pm1 = rand() % 20;
	pm2 = rand() % 4;

	for (j = 0; j < n_petals; j++) {
	    /* Petals are made up beziers */
	    cairo_move_to (cr, 0, 0);
	    cairo_rel_curve_to (cr,
				petal_size, petal_size,
				(pm2+2)*petal_size, petal_size,
				2*petal_size + pm1, 0);
	    cairo_rel_curve_to (cr,
				0 + (pm2*petal_size), -petal_size,
				-petal_size, -petal_size,
				-(2*petal_size + pm1), 0);
	    cairo_close_path (cr);

	    cairo_rotate (cr, 2*M_PI / n_petals);
	}
	cairo_fill_preserve (cr);

	cairo_set_line_width (cr, 1.);
	cairo_set_source_rgb (cr,
			      colors[idx].red,
			      colors[idx].green,
			      colors[idx].blue);
	cairo_stroke (cr);

	petal_size -= rand () % (f->size/8);

	cairo_restore (cr);
    }

    /* Finally draw flower center */
    do {
	idx = rand() % ARRAY_LENGTH (colors);
    } while (idx == last_idx);

    cairo_set_source_rgba (cr,
			   colors[idx].red,
			   colors[idx].green,
			   colors[idx].blue,
			   0.5);

    if (petal_size < 0)
	petal_size = rand() % 10;

    cairo_arc (cr, 0, 0, petal_size, 0, M_PI * 2);
    cairo_fill (cr);

    if (petal_size > 4) {
	cairo_arc (cr, 0, 0, petal_size - 2, 0, M_PI * 2);
	cairo_set_line_width (cr, 2.);
	cairo_set_source_rgba (cr,
			      colors[idx].red,
			      colors[idx].green,
			      colors[idx].blue,
			      .75);
	cairo_stroke (cr);
    }

    f->pattern = cairo_pattern_create_for_surface (cairo_get_target (cr));
    cairo_destroy (cr);
}

static void
fast_flowers_draw (cairo_t *cr, bool opaque)
{
    int i;

    for (i = 0; i< N_FLOWERS; i++) {
	cairo_matrix_t m;
	double mid;

	mid = flowers[i].size / 2.;
	cairo_matrix_init_translate (&m, mid, mid);
	cairo_matrix_rotate (&m, flowers[i].rot);
	cairo_matrix_translate (&m, -mid, -mid);
	cairo_matrix_translate (&m, -flowers[i].x, -flowers[i].y);
	cairo_pattern_set_matrix (flowers[i].pattern, &m);

	cairo_set_source (cr, flowers[i].pattern);
	if (opaque)
		cairo_paint (cr);
	else
		cairo_paint_with_alpha (cr, flowers[i].fade);
    }
}

static void
make_naive_flower (cairo_surface_t *target, flower_t *f, int w, int h)
{
    int i, idx, last_idx = -1;
    int petal_size;

    f->n_groups = rand () % 3 + 1;
    petal_size = PETAL_MIN + rand () % PETAL_VAR;
    f->size = petal_size * 8;

    for (i = 0; i < f->n_groups; i++) {
	f->n_petals[i] = rand () % 5 + 4;
	do {
	    idx = rand () % ARRAY_LENGTH (colors);
	} while (idx == last_idx);
	f->color[i] = idx;
	f->rotation[i] = rand() % 6;
	/* some bezier randomness */
	f->pm1[i] = rand() % 20;
	f->pm2[i] = rand() % 4;
	f->petal_size[i] = petal_size;

	petal_size -= rand () % (f->size/8);
    }
    if (petal_size < 0)
	petal_size = rand() % 10;
    f->petal_size[i] = petal_size;
    do {
	idx = rand() % ARRAY_LENGTH (colors);
    } while (idx == last_idx);
    f->color[i] = idx;



    f->x = (rand() % w - f->size) + f->size/2.;
    f->y = rand() % h;
    f->rot = fmod (rand(), 2 * M_PI);
    f->rv = (fmod (rand(), 5.) + 1) * M_PI * 2. /360.;
    f->v  = rand() % 10 + 2;
    f->fade = fmod (rand(), 100) / 100.;
    f->dfade = .01 * fmod (rand(), 100) / 100.;
}

static void
naive_flowers_draw (cairo_t *cr, bool opaque)
{
    int i, j, n;

    for (n = 0; n< N_FLOWERS; n++) {
	flower_t *f = &flowers[n];
	cairo_matrix_t m;

	if (! opaque) {
		cairo_rectangle (cr, 0, 0, f->size, f->size);
		cairo_clip (cr);
		cairo_push_group (cr);

		cairo_reset_clip (cr);
	} else {
		cairo_save (cr);
		cairo_translate (cr, f->x, f->y);
	}
	cairo_translate (cr, f->size/2., f->size/2.);
	cairo_rotate (cr, f->rot);

	for (i = 0; i < f->n_groups; i++) {
		int n_petals = f->n_petals[i];
		cairo_save (cr);

		cairo_rotate (cr, f->rotation[i]);
		cairo_set_source_rgba (cr,
				       colors[f->color[i]].red,
				       colors[f->color[i]].green,
				       colors[f->color[i]].blue,
				       0.5);

		for (j = 0; j < n_petals; j++) {
			/* Petals are made up beziers */
			cairo_move_to (cr, 0, 0);
			cairo_rel_curve_to (cr,
					    f->petal_size[i], f->petal_size[i],
					    (f->pm2[i]+2)*f->petal_size[i], f->petal_size[i],
					    2*f->petal_size[i] + f->pm1[i], 0);
			cairo_rel_curve_to (cr,
					    0 + (f->pm2[i]*f->petal_size[i]), -f->petal_size[i],
					    -f->petal_size[i], -f->petal_size[i],
					    -(2*f->petal_size[i] + f->pm1[i]), 0);
			cairo_close_path (cr);

			cairo_rotate (cr, 2*M_PI / n_petals);
		}
		cairo_fill_preserve (cr);

		cairo_set_line_width (cr, 1.);
		cairo_set_source_rgb (cr,
				      colors[f->color[i]].red,
				      colors[f->color[i]].green,
				      colors[f->color[i]].blue);
		cairo_stroke (cr);

		cairo_restore (cr);
	}

	/* Finally draw flower center */
	cairo_set_source_rgba (cr,
			       colors[f->color[i]].red,
			       colors[f->color[i]].green,
			       colors[f->color[i]].blue,
			       0.5);

	cairo_arc (cr, 0, 0, f->petal_size[i], 0, M_PI * 2);
	cairo_fill (cr);

	if (f->petal_size[i] > 4) {
		cairo_arc (cr, 0, 0, f->petal_size[i] - 2, 0, M_PI * 2);
		cairo_set_line_width (cr, 2.);
		cairo_set_source_rgba (cr,
				       colors[f->color[i]].red,
				       colors[f->color[i]].green,
				       colors[f->color[i]].blue,
				       .75);
		cairo_stroke (cr);
	}
	if (! opaque) {
		cairo_pop_group_to_source(cr);
		cairo_reset_clip (cr);

		cairo_matrix_init_translate (&m, -f->x, -f->y);
		cairo_pattern_set_matrix (cairo_get_source(cr), &m);
		cairo_paint_with_alpha (cr, f->fade);
	} else
		cairo_restore (cr);
    }
}

static void
flowers_update (int width, int height)
{
    int i;

    for (i = 0; i< N_FLOWERS; i++) {
	flowers[i].y   += flowers[i].v;
	if (flowers[i].y > height || flowers[i].y + flowers[i].size < 0)
	    flowers[i].v = -flowers[i].v;

	flowers[i].fade += flowers[i].dfade;
	if (flowers[i].fade < 0. || flowers[i].fade > 1.)
	    flowers[i].dfade = -flowers[i].dfade;

	flowers[i].rot += flowers[i].rv;
    }
}

int main (int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;

	double delta;
	int frame = 0;
	int frames = 0;
	int benchmark;
	void (*draw) (cairo_t *, bool);
	int naive, opaque;
	int i;

	device = device_open(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	naive = opaque = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--naive") == 0)
			naive = 1;
		else if (strcmp(argv[i], "--opaque") == 0)
			opaque = 1;
	}


	for (i = 0; i< N_FLOWERS; i++)
		(naive ? make_naive_flower : make_fast_flower) (device->scanout,
								&flowers[i],
								device->width,
								device->height);
	draw = naive ? naive_flowers_draw : fast_flowers_draw;

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr;

		cr = cairo_create(fb->surface);

		cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		draw(cr, opaque);

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec)
			fps_draw(cr, device->name, &last_fps, &now);
		last_fps = now;

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		flowers_update(device->width, device->height);

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
