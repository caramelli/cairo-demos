#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"

static struct source {
	cairo_surface_t *surface;
	int width, height;
} *sources;
static int num_sources;
static size_t in_pixels, out_pixels;

int preload = 1;
int prescale = 0;

static void
fps_draw (cairo_t *cr, const char *name,
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

    snprintf (buf, sizeof (buf), "%s: %.1f fps", name, 1. / avg);
    cairo_set_font_size (cr, 18);
    cairo_text_extents (cr, buf, &extents);

    cairo_rectangle (cr, 4-1, 4-1, extents.width+2, extents.height+2);
    cairo_set_source_rgba (cr, .0, .0, .0, .85);
    cairo_fill (cr);

    cairo_move_to (cr, 4 - extents.x_bearing, 4 - extents.y_bearing);
    cairo_set_source_rgb (cr, .95, .95, .95);
    cairo_show_text (cr, buf);
}

static int load_sources_file(const char *filename,
			     struct device *device,
			     cairo_surface_t *target)
{
	GdkPixbuf *pb;
	cairo_surface_t *surface, *image;
	cairo_status_t status;
	cairo_t *cr;
	int width, height;
	int ok = 0;

	pb = gdk_pixbuf_new_from_file(filename, NULL);

	if (pb == NULL)
		return 0;

	image = _cairo_image_surface_create_from_pixbuf(pb);
	g_object_unref(pb);

	width = cairo_image_surface_get_width(image);
	height = cairo_image_surface_get_height(image);

	if (!preload && !prescale) {
		struct source *s;

		s = realloc(sources,
			    (num_sources+1)*sizeof(struct source));
		if (s == NULL) {
			cairo_surface_destroy(image);
			return 0;
		}

		s[num_sources].surface = image;
		s[num_sources].width = width;
		s[num_sources].height = height;
		num_sources++;
		sources = s;

		in_pixels  += width * height;
		out_pixels += width * height;
		return 1;
	}

	if (prescale) {
		double delta;

		if (height * device->width > width * device->height)
			delta = device->width / (double)width;
		else
			delta = device->height / (double)height;

		width  *= delta;
		height *= delta;
	}

	surface = cairo_surface_create_similar(preload ? target : image,
					       CAIRO_CONTENT_COLOR,
					       width, height);

	cr = cairo_create(surface);
	cairo_scale(cr,
		    width / (float)cairo_image_surface_get_width(image),
		    height / (float)cairo_image_surface_get_height(image));
	cairo_set_source_surface(cr, image, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	status = cairo_status(cr);
	cairo_destroy(cr);

	if (status == CAIRO_STATUS_SUCCESS) {
		struct source *s;

		s = realloc(sources,
			    (num_sources+1)*sizeof(struct source));
		if (s) {
			s[num_sources].surface =
				cairo_surface_reference(surface);
			s[num_sources].width = width;
			s[num_sources].height = height;
			num_sources++;
			sources = s;

			in_pixels += cairo_image_surface_get_width(image)*cairo_image_surface_get_height(image);
			out_pixels += width * height;
			ok = 1;
		}
	}

	cairo_surface_destroy(image);
	cairo_surface_destroy(surface);

	return ok;
}

static void load_sources_dir(const char *path,
			     struct device *device,
			     cairo_surface_t *target)
{
	struct dirent *de;
	DIR *dir;
	int count = 0;

	dir = opendir(path);
	if (dir == NULL)
		return;

	while ((de = readdir(dir))) {
		struct stat st;
		gchar *filename;
		if (de->d_name[0] == '.')
			continue;

		filename = g_build_filename(path, de->d_name, NULL);

		if (stat(filename, &st)) {
			g_free(filename);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			load_sources_dir(filename, device, target);
			g_free(filename);
			continue;
		}

		count += load_sources_file(filename, device, target);
		g_free(filename);
	}
	closedir(dir);

	if (count)
		printf("Loaded %d images from %s\n", count, path);
}

static void load_sources(const char *path,
			 struct device *device,
			 cairo_surface_t *target)
{
	struct stat st;

	if (stat(path, &st))
		return;

	if (S_ISDIR(st.st_mode))
		load_sources_dir(path, device, target);
	else
		load_sources_file(path, device, target);
}

int main(int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;
	int frames, n;
	int show_fps = 1;

	device = device_open(argc, argv);

	g_type_init();

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--images") == 0) {
			n++;
		} else if (strcmp (argv[n], "--prescale") == 0) {
			prescale = 1;
		} else if (strcmp (argv[n], "--no-preload") == 0) {
			preload = 0;
		} else if (strcmp (argv[n], "--hide-fps") == 0) {
			show_fps = 0;
		}
	}

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--images") == 0) {
			load_sources(argv[++n], device,
				     device->get_framebuffer(device)->surface);
			n++;
		}
	}
	if (num_sources == 0)
		load_sources("/usr/share/backgrounds", device,
			     device->get_framebuffer(device)->surface);

	printf("Loaded %d images, %d/%d pixels in total\n",
	       num_sources, out_pixels, in_pixels);
	if (num_sources == 0)
		return 0;

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	n = frames = 0;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		struct source *source = &sources[n++ % num_sources];
		cairo_t *cr;
		double delta;


		if (source->height * device->width > source->width * device->height)
			delta = device->width / (double)source->width;
		else
			delta = device->height / (double)source->height;

		cr = cairo_create (fb->surface);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_translate(cr, device->width/2, device->height/2);
		cairo_rotate(cr, (n%360)*M_PI/180);
		cairo_scale(cr, delta, delta);
		cairo_translate(cr, -source->width/2, -source->height/2);
		cairo_set_source_surface(cr, source->surface, 0, 0);
		cairo_pattern_set_extend(cairo_get_source(cr),
					 CAIRO_EXTEND_NONE);
		cairo_pattern_set_filter(cairo_get_source(cr),
					 CAIRO_FILTER_BILINEAR);
		cairo_identity_matrix(cr);
		cairo_paint(cr);

		gettimeofday(&now, NULL);
		if (show_fps) {
			if (last_fps.tv_sec)
				fps_draw(cr, device->name, &last_fps, &now);
			last_fps = now;
		}

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		frames++;
		delta = now.tv_sec - last_tty.tv_sec;
		delta += (now.tv_usec - last_tty.tv_usec)*1e-6;
		if (delta >  5) {
			printf("%.2f fps\n", frames/delta);
			last_tty = now;
			frames = 0;
		}
	} while (1);

	return 0;
}
