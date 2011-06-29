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
ifneq ($(GLX),0)
DEFINES+=-DHAVE_GLX=1
SOURCES+=glx.c
REQUIRES+=cairo-gl
else
DEFINES+=-DHAVE_GLX=0
endif

all: spiral-demo

ifeq ($(shell pkg-config --exists poppler-glib && echo 1), 1)
all: poppler-demo
REQUIRES+=poppler-glib
endif

CFLAGS:=$(shell pkg-config --cflags $(REQUIRES)) -Wall -g3
LIBS:=$(shell pkg-config --libs $(REQUIRES))

spiral-demo: spiral-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ spiral-demo.c $(SOURCES) $(LIBS)
poppler-demo: poppler-demo.c $(SOURCES) demo.h Makefile
	$(CC) $(DEFINES) $(CFLAGS) -o $@ poppler-demo.c $(SOURCES) $(LIBS)
clean:
	rm -f *-demo

