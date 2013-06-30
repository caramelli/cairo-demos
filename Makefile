SOURCES:=demo.c png.c
REQUIRES:=cairo

DRM:=0
ifneq ($(DRM),0)
DEFINES+=-DHAVE_DRM=1
SOURCES+=drm.c
REQUIRES+=cairo-drm libdrm
else
DEFINES+=-DHAVE_DRM=0
endif

XLIB:=$(shell pkg-config --exists cairo-xlib && echo 1 || echo 0)
ifneq ($(XLIB),0)
DEFINES+=-DHAVE_XLIB=1 -DHAVE_XIMAGE=1 -DHAVE_XSHM=1
SOURCES+=xlib.c ximage.c xshm.c
REQUIRES+=cairo-xlib xext
else
DEFINES+=-DHAVE_XLIB=0 -DHAVE_XIMAGE=0 -DHAVE_XSHM=0
endif

XCB:=$(shell pkg-config --exists cairo-xcb && echo 1 || echo 0)
ifneq ($(XCB),0)
DEFINES+=-DHAVE_XCB=1
SOURCES+=xcb.c
REQUIRES+=cairo-xcb
else
DEFINES+=-DHAVE_XCB=0
endif

GLX:=$(shell pkg-config --exists cairo-gl && echo 1 || echo 0)
GLX:=0
ifneq ($(GLX),0)
DEFINES+=-DHAVE_GLX=1
SOURCES+=glx.c
REQUIRES+=cairo-gl
else
DEFINES+=-DHAVE_GLX=0
endif

COGL:=$(shell pkg-config --exists cairo-cogl && echo 1 || echo 0)
ifneq ($(COGL),0)
DEFINES+=-DHAVE_COGL=1
SOURCES+=cogl.c
REQUIRES+=cairo-cogl
else
DEFINES+=-DHAVE_COGL=0
endif

SKIA:=$(shell pkg-config --exists cairo-skia && echo 1 || echo 0)
ifneq ($(SKIA),0)
DEFINES+=-DHAVE_SKIA=1
SOURCES+=skia.c
REQUIRES+=cairo-skia
else
DEFINES+=-DHAVE_SKIA=0
endif

all: spiral-demo tiger-demo fish-demo flowers-demo gears-demo gradient-demo chart-demo waterfall-demo dragon-demo pythagoras-demo wave-demo sierpinski-demo maze-demo bubble-demo

ifeq ($(shell pkg-config --exists poppler-glib && echo 1), 1)
all: poppler-demo
REQUIRES+=poppler-glib
endif

ifeq ($(shell pkg-config --exists gdk-pixbuf-2.0 && echo 1), 1)
all: spinner-demo slideshow-demo
REQUIRES+=gdk-pixbuf-2.0
DEFINES+=-DHAVE_GDK_PIXBUF
endif

CFLAGS:=$(shell pkg-config --cflags $(REQUIRES)) -Wall -g3
LIBS:=$(shell pkg-config --libs $(REQUIRES)) -lpng -lm

spinner-demo: spinner-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ spinner-demo.c $(SOURCES) $(LIBS)
spiral-demo: spiral-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ spiral-demo.c $(SOURCES) $(LIBS)
slideshow-demo: slideshow-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ slideshow-demo.c $(SOURCES) $(LIBS)
poppler-demo: poppler-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ poppler-demo.c $(SOURCES) $(LIBS)
tiger-demo: tiger-demo.c $(SOURCES) demo.h Makefile tiger.inc
	$(CC) $(DEFINES) $(CFLAGS) -o $@ tiger-demo.c $(SOURCES) $(LIBS)
fish-demo: fish-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ fish-demo.c $(SOURCES) $(LIBS)
fish2-demo: fish2-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ fish2-demo.c $(SOURCES) $(LIBS)
bubble-demo: bubble-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ bubble-demo.c $(SOURCES) $(LIBS)
flowers-demo: flowers-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ flowers-demo.c $(SOURCES) $(LIBS)
gears-demo: gears-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ gears-demo.c $(SOURCES) $(LIBS)
gradient-demo: gradient-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ gradient-demo.c $(SOURCES) $(LIBS)
chart-demo: chart-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ chart-demo.c $(SOURCES) $(LIBS)
waterfall-demo: waterfall-demo.c $(SOURCES) demo.h list.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ waterfall-demo.c $(SOURCES) $(LIBS)
dragon-demo: dragon-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ dragon-demo.c $(SOURCES) $(LIBS)
pythagoras-demo: pythagoras-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ pythagoras-demo.c $(SOURCES) $(LIBS)
wave-demo: wave-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ wave-demo.c $(SOURCES) $(LIBS)
sierpinski-demo: sierpinski-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ sierpinski-demo.c $(SOURCES) $(LIBS)
maze-demo: maze-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ maze-demo.c $(SOURCES) $(LIBS)
clean:
	rm -f *-demo

