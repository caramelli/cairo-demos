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

static int load_source(const char *filename, struct device *device)
{
	GdkPixbuf *pb;
	cairo_surface_t *target, *surface, *image;
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

		s = (struct source *)realloc(sources, (num_sources+1)*sizeof(struct source));
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

	target = device->get_framebuffer(device)->surface;
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

		s = (struct source *)realloc(sources, (num_sources+1)*sizeof(struct source));
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

static int done;
static void signal_handler(int sig)
{
	done = sig;
}

int main(int argc, char **argv)
{
	struct device *device;
	struct timeval start, last_tty, last_fps, now;
	const char *filename = "images/4000x4000/202672-4000x4000.jpg";
	int frames, n;
	int frame = 0;
	double factor = 1.0125;
	double zoom = 0.75;
	int show_fps = 1;
	const char *version;
	int benchmark;

	device = device_open(argc, argv);
	version = device_show_version(argc, argv);
	benchmark = device_get_benchmark(argc,argv);
	if (benchmark == 0)
		benchmark = 20;

	signal(SIGHUP, signal_handler);

	g_type_init();

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--image") == 0) {
			filename = argv[++n];
		} else if (strncmp (argv[n], "--image=", 8) == 0) {
			filename = argv[n] + 8;
		} else if (strcmp (argv[n], "--prescale") == 0) {
			prescale = 1;
		} else if (strcmp (argv[n], "--no-preload") == 0) {
			preload = 0;
		} else if (strcmp (argv[n], "--hide-fps") == 0) {
			show_fps = 0;
		}
	}

	if (!load_source(filename, device))
		return 1;

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
		delta *= zoom;

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

		if (++frame % 512 == 0)
			factor = 1./factor;
		zoom *= factor;

		gettimeofday(&now, NULL);
		if (benchmark < 0 && show_fps) {
			if (last_fps.tv_sec)
				fps_draw(cr, device->name, version,
					 &last_fps, &now);
			last_fps = now;
		}

		cairo_destroy(cr);

		fb->show (fb);
		fb->destroy (fb);

		if (0) {
			frames++;
			delta = now.tv_sec - last_tty.tv_sec;
			delta += (now.tv_usec - last_tty.tv_usec)*1e-6;
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
				printf("spinner: %.2f fps\n",
				       frame / delta);
				break;
			}
		}
	} while (!done);

	if (benchmark < 0) {
		struct framebuffer *fb = device->get_framebuffer (device);
		struct source *source = &sources[0];
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

		cairo_destroy(cr);

		fps_finish(fb, device->name, version);
		fb->show (fb);
		fb->destroy (fb);
		pause();
	}

	return 0;
}
