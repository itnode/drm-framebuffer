CC      ?= gcc
CFLAGS  ?= -O0 -ggdb -pedantic -Wall -I /usr/include/libdrm
LDFLAGS ?= -ldrm

OBJ = main.o framebuffer.o
PROGNAME = drm-framebuffer

exec_prefix ?= /usr
bindir ?= $(exec_prefix)/bin

all: drm-framebuffer drmfb_daemon

drm-framebuffer: main.o framebuffer.o
	$(CC) $(CFLAGS) -o drm-framebuffer main.o framebuffer.o $(LDFLAGS)

drmfb_daemon: drmfb_daemon.c framebuffer.o
	$(CC) $(CFLAGS) -o drmfb_daemon drmfb_daemon.c framebuffer.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	@echo "Clean object files"
	@rm -f *.o drm-framebuffer drmfb_daemon
