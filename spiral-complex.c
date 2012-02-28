#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"

static struct source{
	cairo_surface_t *surface;
	int width, height;
} *sources;
static int num_sources;
static size_t in_pixels, out_pixels;

int prescale = 0;

static int load_sources_file(const char *filename, cairo_surface_t *target)
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
	if (prescale) {
		int max = width > height ? width : height;
		if (max > 512) {
			if (width > height) {
				height = height * 512 / width;
				width = 512;
			} else {
				width = width * 512 / height;
				height = 512;
			}
		}
	}

	surface = cairo_surface_create_similar(target,
					       CAIRO_CONTENT_COLOR,
					       //cairo_surface_get_content(image),
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

static void load_sources_dir(const char *path, cairo_surface_t *target)
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
			load_sources_dir(filename, target);
			g_free(filename);
			continue;
		}

		count += load_sources_file(filename, target);
		g_free(filename);
	}
	closedir(dir);

	if (count)
		printf("Loaded %d images from %s\n", count, path);
}

static void load_sources(const char *path, cairo_surface_t *target)
{
	struct stat st;

	if (stat(path, &st))
		return;

	if (S_ISDIR(st.st_mode))
		load_sources_dir(path, target);
	else
		load_sources_file(path, target);
}

int main(int argc, char **argv)
{
	struct device *device;
	const char *path = "/usr/share/backgrounds";
	float sincos_lut[360];
	struct timeval start, last_tty, last_fps, now;
	int theta, frames, n;
	int show_path = 1;
	int show_outline = 1;
	int show_images = 1;
	int show_fps = 1;
	int fade = 1;

	device = device_open(argc, argv);

	for (n = 1; n < argc; n++) {
		if (strcmp (argv[n], "--images") == 0) {
			path = argv[n+1];
			n++;
	    } else if (strcmp (argv[n], "--hide-path") == 0) {
		show_path = 0;
	    } else if (strcmp (argv[n], "--hide-outline") == 0) {
		show_outline = 0;
	    } else if (strcmp (argv[n], "--hide-images") == 0) {
		show_images = 0;
	    } else if (strcmp (argv[n], "--hide-fps") == 0) {
		show_fps = 0;
	    } else if (strcmp (argv[n], "--no-fade") == 0) {
		fade = 0;
	    }
	}

	g_type_init();

	for (theta = 0; theta < 360; theta+= 2) {
		sincos_lut[theta+0] = sin(theta/180. * M_PI);
		sincos_lut[theta+1] = cos(theta/180. * M_PI);
	}

	load_sources(path, device->get_framebuffer(device)->surface);
	printf("Loaded %d images, %ld/%ld pixels in total from %s\n",
	       num_sources, (long)out_pixels, (long)in_pixels, path);

	gettimeofday(&start, 0); now = last_tty = last_fps = start;
	frames = 0;
	do {
		struct framebuffer *fb;
		cairo_t *cr;
		double delta;
		int width = device->width;
		int height = device->height;
		int rotation;

		fb = device->get_framebuffer (device);
		cr = cairo_create (fb->surface);

		gettimeofday(&now, 0);
		delta = now.tv_sec - start.tv_sec;
		delta += (now.tv_usec - start.tv_usec)*1e-6;

		rotation = floor(50*delta);

		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		if (show_path) {
			cairo_pattern_t *p;

			if (fade) {
				p = cairo_pattern_create_radial(width/2, height/2, 0,
								width/2, height/2, MAX(width, height)/2);
				cairo_pattern_add_color_stop_rgb(p, 0, 1, 1, 1);
				cairo_pattern_add_color_stop_rgb(p, 1, 0, 0, 0);
			} else {
				p = cairo_pattern_create_rgb(1,1,1);
			}
			cairo_set_source(cr, p);
			cairo_pattern_destroy (p);

			cairo_set_line_width(cr, 5);
			cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

			cairo_move_to(cr, width/2, height/2);
			for (theta = 0; theta < 2000; theta += 5) {
				double dx = theta*sincos_lut[(((rotation+theta)%360)&~1)+0]/2;
				double dy = theta*sincos_lut[(((rotation+theta)%360)&~1)+1]/2;
				cairo_line_to(cr, width/2+dx, height/2+dy);
			}
			cairo_stroke(cr);
		}

		if (num_sources) {
			int step = 50;
			int image = num_sources;
			for (theta = 0; theta < 2000; theta += step) {
				double dx = theta*sincos_lut[(((rotation+theta)%360)&~1)+0]/2;
				double dy = theta*sincos_lut[(((rotation+theta)%360)&~1)+1]/2;
				double spin = 500*delta/(step+theta);

				struct source *source = &sources[image++%num_sources];
				int w = 2*ceil(step/M_SQRT2/5*4 * (1+theta/360.));
				int h = 2*ceil(step/M_SQRT2/5*3 * (1+theta/360.));

				cairo_save(cr);
				if (show_images) {
					cairo_translate(cr, width/2+dx, height/2+dy);
					cairo_rotate(cr, M_PI/2+spin+(rotation+theta)/180.*M_PI);
					cairo_scale(cr,
						    w/(double)source->width,
						    h/(double)source->height);
					cairo_set_source_surface(cr, source->surface,
								 -source->width/2.,
								 -source->height/2.);
					cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_NONE);
					cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
					cairo_identity_matrix(cr);
					if (fade) {
						cairo_paint_with_alpha(cr,
						 1. - 2*sqrt((dx*dx + dy*dy)/(width*width + height*height)));
					} else
						cairo_paint(cr);
				}

				if (show_outline) {
					double alpha = 1;
					if (fade)
						 alpha = 1. - 2*sqrt((dx*dx + dy*dy)/(width*width + height*height));
					cairo_set_source_rgb(cr, alpha, alpha, alpha);
					cairo_set_line_width(cr, 2);

					cairo_translate(cr, width/2+dx, height/2+dy);
					cairo_rotate(cr, M_PI/2+spin+(rotation+theta)/180.*M_PI);
					cairo_rectangle(cr, -w/2, -h/2, w, h);

					cairo_identity_matrix(cr);
					cairo_stroke(cr);
				}
				cairo_restore(cr);

				step += 5;
			}
		}

		if (show_fps) {
			if (last_fps.tv_sec)
				fps_draw(cr, device->name, NULL, &last_fps, &now);
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
