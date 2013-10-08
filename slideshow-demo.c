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

	image = surface_create_from_pixbuf(preload ? target : NULL, pb);
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
		width = device->width;
		height = device->height;
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

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main(int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;
	int frame, n;
	int transition = 1;
	double divide = 1.;
	const char *version;
	int benchmark;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	benchmark = device_get_benchmark(argc, argv);
	if (benchmark == 0)
		benchmark = 20;

	g_type_init();

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--images") == 0) {
			n++;
		} else if (strcmp (argv[n], "--prescale") == 0) {
			prescale = 1;
		} else if (strcmp (argv[n], "--no-preload") == 0) {
			preload = 0;
		} else if (strcmp (argv[n], "--no-transition") == 0) {
			transition = 0;
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

	printf("Loaded %d images, %ld/%ld pixels in total\n",
	       num_sources, (long)out_pixels, (long)in_pixels);
	if (num_sources == 0)
		return 0;

	signal(SIGHUP, signal_handler);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	n = frame = 0;
	do {
		struct framebuffer *fb = device->get_framebuffer (device);
		struct source *left = &sources[n % num_sources];
		cairo_t *cr;
		double delta;

		cr = cairo_create (fb->surface);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_scale(cr,
			    device->width/(double)left->width,
			    device->height/(double)left->height);
		cairo_set_source_surface(cr, left->surface, 0, 0);
		cairo_paint(cr);
		cairo_identity_matrix(cr);

		if (transition) {
			struct source *right = &sources[(n + 1) % num_sources];
			cairo_pattern_t *mask;

			mask = cairo_pattern_create_linear(0, 0, device->width, device->height);
			cairo_pattern_add_color_stop_rgba(mask, divide-.2, 0, 0, 0, 0);
			cairo_pattern_add_color_stop_rgba(mask, divide, 0, 0, 0, 1);

			cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OUT);
			cairo_set_source(cr, mask);
			cairo_paint(cr);

			cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
			cairo_scale(cr,
				    device->width/(double)right->width,
				    device->height/(double)right->height);
			cairo_set_source_surface(cr, right->surface, 0, 0);
			cairo_identity_matrix(cr);
			cairo_mask(cr, mask);
			cairo_pattern_destroy(mask);

			divide -= 0.05;
			if (divide < 0)
				divide = 1., n++;
		} else
			n++;

		gettimeofday(&now, NULL);
		if (benchmark < 0 && last_fps.tv_sec) {
			cairo_reset_clip (cr);
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
				printf("fish: %.2f fps\n", frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		cairo_t *cr = cairo_create(fb->surface);

		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);
		cairo_destroy(cr);

		fps_finish(fb, device->name, version, "slideshow");
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
