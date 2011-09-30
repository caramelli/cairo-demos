SOURCES:=demo.c xlib.c ximage.c
REQUIRES:=cairo-xlib xext gdk-pixbuf-2.0
DEFINES:=-DHAVE_XLIB=1 -DHAVE_XIMAGE=1

DRM:=0
ifneq ($(DRM),0)
DEFINES+=-DHAVE_DRM=1
SOURCES+=drm.c
REQUIRES+=cairo-drm libdrm
else
DEFINES+=-DHAVE_DRM=0
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

all: spinner-demo spiral-demo slideshow-demo tiger-demo fish-demo flowers-demo gears-demo chart-demo

ifeq ($(shell pkg-config --exists poppler-glib && echo 1), 1)
all: poppler-demo
REQUIRES+=poppler-glib
endif

CFLAGS:=$(shell pkg-config --cflags $(REQUIRES)) -Wall -g3
LIBS:=$(shell pkg-config --libs $(REQUIRES))

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
flowers-demo: flowers-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ flowers-demo.c $(SOURCES) $(LIBS)
gears-demo: gears-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ gears-demo.c $(SOURCES) $(LIBS)
chart-demo: chart-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ chart-demo.c $(SOURCES) $(LIBS)
clean:
	rm -f *-demo

