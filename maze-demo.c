#include "demo.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

enum { N = 1, S = 2, W = 4, E = 8, V = 16, D = 32};

static int w = 40;
static int h = 20;
static int avail;
static uint8_t **cell;

#define each(i, x, y) for (i = x; i <= y; i++)
inline static int max(int a, int b) { return a >= b ? a : b; }
inline static int min(int a, int b) { return b >= a ? a : b; }

static int irand(int n)
{
	int r, rmax = n * (RAND_MAX / n);
	while ((r = rand()) >= rmax);
	return r / (RAND_MAX/n);
}

static void show(cairo_t *cr, int width, int height, int use_mask)
{
	int i, j;
	int dx, dy, step;

	dx = width/(w+2);
	dy = height/(h+2);
	step = min(dx, dy);
	step = max(step, 4);

	dx = (width-step*w)/2;
	dy = (height-step*h)/2;

	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	cairo_translate(cr, dx, dy);
	cairo_scale(cr, step, step);

	cairo_set_source_rgb(cr, 0.2, 0.2, 0.7);
	cairo_arc(cr, .5, .5, 1/3., 0, 2*M_PI);
	cairo_fill(cr);
	cairo_arc(cr, (w-.5), (h-.5), 1/3., 0, 2*M_PI);
	cairo_fill(cr);

	each(i, 0, 2 * h) {
		each(j, 0, 2 * w) {
			uint8_t c = cell[i][j];
			if (c > V)
				continue;

			if ((c & (N | S)) == (N | S)) {
				cairo_move_to(cr, j/2, i/2);
				cairo_line_to(cr, j/2, i/2+1);
			}
			if ((c & (W | E)) == (W | E)) {
				cairo_move_to(cr, j/2, i/2);
				cairo_line_to(cr, j/2+1, i/2);
			}
		}
	}

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_save(cr); {
		cairo_identity_matrix(cr);
		cairo_set_line_width(cr, 2);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_stroke(cr);
	} cairo_restore (cr);

	if (use_mask) {
		cairo_surface_t *mask;
		cairo_t *tmp;

		mask = cairo_surface_create_similar(cairo_get_target(cr),
						    CAIRO_CONTENT_ALPHA,
						    step, step);
		tmp = cairo_create(mask);
		cairo_arc(tmp, step/2., step/2., step/5., 0, 2 * M_PI);
		cairo_set_operator(tmp, CAIRO_OPERATOR_ADD);
		cairo_fill(tmp);
		cairo_destroy(tmp);

		cairo_save(cr);
		cairo_identity_matrix(cr);
		cairo_set_source_rgb(cr, 0.70, 0.75, 0.75);
		each(i, 0, 2 * h) {
			each(j, 0, 2 * w) {
				uint8_t c = cell[i][j];
				if (c & V)
					cairo_mask_surface(cr, mask,
							   j/2*step + dx,
							   i/2*step + dy);
			}
		}
		cairo_restore(cr);
		cairo_surface_destroy(mask);

		mask = cairo_surface_create_similar(cairo_get_target(cr),
						    CAIRO_CONTENT_ALPHA,
						    step, step);
		tmp = cairo_create(mask);
		cairo_arc(tmp, step/2., step/2., step/4., 0, 2 * M_PI);
		cairo_set_operator(tmp, CAIRO_OPERATOR_ADD);
		cairo_fill(tmp);
		cairo_destroy(tmp);

		cairo_save(cr);
		cairo_identity_matrix(cr);
		cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
		each(i, 0, 2 * h) {
			each(j, 0, 2 * w) {
				uint8_t c = cell[i][j];
				if (c & D)
					cairo_mask_surface(cr, mask,
							   j/2*step + dx,
							   i/2*step + dy);
			}
		}
		cairo_surface_destroy(mask);
		cairo_restore(cr);
	} else {
		each(i, 0, 2 * h) {
			each(j, 0, 2 * w) {
				uint8_t c = cell[i][j];
				if (c & V) {
					cairo_arc(cr, j/2+.5, i/2+.5, 1/5., 0, 2*M_PI);
					cairo_set_source_rgb(cr, 0.70, 0.75, 0.75);
					cairo_fill(cr);
				}
				if (c & D) {
					cairo_arc(cr, j/2+.5, i/2+.5, 1/4., 0, 2*M_PI);
					cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
					cairo_fill(cr);
				}
			}
		}
	}

	each(i, 0, 2 * h) {
		each(j, 0, 2 * w) {
			uint8_t c = cell[i][j];
			if ((c & V) == 0)
				continue;
			if (c & N) {
				cairo_move_to(cr, j/2+.5, i/2);
				cairo_line_to(cr, j/2+.5, i/2+.5);
			}
			if (c & S) {
				cairo_move_to(cr, j/2+.5, i/2+.5);
				cairo_line_to(cr, j/2+.5, i/2+1);
			}
			if (c & W) {
				cairo_move_to(cr, j/2, i/2+.5);
				cairo_line_to(cr, j/2+.5, i/2+.5);
			}
			if (c & E) {
				cairo_move_to(cr, j/2+.5, i/2+.5);
				cairo_line_to(cr, j/2+1, i/2+.5);
			}
		}
	}

	cairo_set_source_rgb(cr, 0.2, 0.7, 0.2);
	cairo_save(cr); {
		cairo_identity_matrix(cr);
		cairo_set_line_width(cr, 1);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_stroke(cr);
	} cairo_restore (cr);

	cairo_identity_matrix(cr);
}

static int dirs[4][2] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
static void walk(int x, int y)
{
	int i, t, x1, y1, d[4] = { 0, 1, 2, 3 };

	cell[y][x] |= V;
	avail--;

	for (x1 = 3; x1; x1--)
		if (x1 != (y1 = irand(x1 + 1)))
			i = d[x1], d[x1] = d[y1], d[y1] = i;

	for (i = 0; avail && i < 4; i++) {
		x1 = x + dirs[ d[i] ][0], y1 = y + dirs[ d[i] ][1];

		if (cell[y1][x1] & V)
		       	continue;

		/* break walls */
		if (x1 == x) {
			t = (y + y1) / 2;
			cell[t][x+1] &= ~W;
		       	cell[t][x] &= ~(E|W);
		       	cell[t][x-1] &= ~E;
		} else if (y1 == y) {
			t = (x + x1)/2;
			cell[y-1][t] &= ~S;
		       	cell[y][t] &= ~(N|S);
		       	cell[y+1][t] &= ~N;
		}
		walk(x1, y1);
	}
}

static int solve(int x, int y, int tox, int toy, int *depth)
{
	int i;

	cell[y][x] |= V;
	if (x == tox && y == toy)
	       	return 1;

	if (--*depth == 0)
		return -1;

	each(i, 0, 3) {
		int x1 = x + dirs[i][0];
	       	int y1 = y + dirs[i][1];
		int ret;

		if (cell[y1][x1] & V)
		       	continue;

		if (i & 1) {
			int t = (y + y1)/2;
			if (cell[t][x] & ~D)
				continue;

			ret = solve(x1, y1, tox, toy, depth);
			if (ret == 0)
				continue;

			cell[t-1][x] |= S;
			cell[t+1][x] |= N;
		} else {
			int t = (x + x1)/2;
			if (cell[y][t] & ~D)
			       	continue;

			ret = solve(x1, y1, tox, toy, depth);
			if (ret == 0)
				continue;

			cell[y][t-1] |= E;
			cell[y][t+1] |= W;
		}

		return ret;
	}

	if (--*depth == 0)
		return -1;

	cell[y][x] = D;
	return 0;
}

static void maze_create(void)
{
	int i, h2 = 2 * h + 2, w2 = 2 * w + 2;
	uint8_t **p;

	p = (uint8_t **)calloc(sizeof(uint8_t*) * (h2 + 2) + w2 * h2 + 1, 1);

	p[1] = (uint8_t*)(p + h2 + 2) + 1;
	each(i, 2, h2) p[i] = p[i-1] + w2;
	p[0] = p[h2];
	cell = &p[1];
}

static void maze_reset(void)
{
	int i, j;
	int h2 = 2 * h + 2, w2 = 2 * w + 2;

	each(i, 0, 2*h) each(j, 0, 2*w) cell[i][j] = 0;

	each(i, -1, 2 * h + 1) cell[i][-1] = cell[i][w2 - 1] = V;
	each(j, 0, 2 * w) cell[-1][j] = cell[h2 - 1][j] = V;
	each(i, 0, h) each(j, 0, 2 * w) cell[2*i][j] |= E|W;
	each(i, 0, 2 * h) each(j, 0, w) cell[i][2*j] |= N|S;
	each(j, 0, 2 * w) cell[0][j] &= ~N, cell[2*h][j] &= ~S;
	each(i, 0, 2 * h) cell[i][0] &= ~W, cell[i][2*w] &= ~E;

	avail = w * h;
	walk(irand(2) * 2 + 1, irand(h) * 2 + 1);
}

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main (int argc, char **argv)
{
	struct device *device = device_open(argc, argv);
	struct timeval start, last_tty, last_fps, now, complete;
	int iterations = 1;

	double delta;
	int frame = 0;
	int show_fps = 1;
	int use_mask = 0;
	int benchmark;
	const char *version = device_show_version(argc, argv);
	//enum clip clip = device_get_clip(argc, argv);

	int n;

	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;
	if (benchmark > 0)
		show_fps = 0;

	signal(SIGHUP, signal_handler);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--hide-fps") == 0)
			show_fps = 0;
		else if (strcmp (argv[n], "--use-mask") == 0)
			use_mask = 1;
	}

	maze_create();
	maze_reset();
	complete.tv_sec = 0;

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);
		int i, j, ret;

		each(i, 0, 2 * h) each(j, 0, 2 * w)
			if (cell[i][j] & (V|D)) cell[i][j] = 0;
		i = iterations++;
		ret = solve(1, 1, 2 * w - 1, 2 * h - 1, &i);
		show(cr, device->width, device->height, use_mask);

		gettimeofday(&now, NULL);
		if (show_fps && last_fps.tv_sec) {
			fps_draw(cr, device->name, version, &last_fps, &now);
		}
		last_fps = now;

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		frame++;
		if (benchmark > 0) {
			delta = now.tv_sec - start.tv_sec;
			delta += (now.tv_usec - start.tv_usec)*1e-6;
			if (delta > benchmark) {
				printf("maze: %.2f fps\n", frame / delta);
				break;
			}
		}

		if (ret > 0) {
			if (complete.tv_sec == 0)
				complete = now;
			delta = now.tv_sec - complete.tv_sec;
			delta += (now.tv_usec - complete.tv_usec)*1e-6;
			if (delta > 1.5) {
				maze_reset();
				iterations = 1;
				complete.tv_sec = 0;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		show(cr, device->width, device->height, use_mask);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version, "maze");
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
