PLATFORM := $(shell uname -o)

CFLAGS := $(CFLAGS) -Wall $(shell sdl-config --cflags)
LIBS := $(LIBS) $(shell sdl-config --libs)
SRCS := rgbm.c sdl_display.c

ifeq ($(PLATFORM),Windows)

CC := i686-w64-mingw32-gcc
WINAMPAPI_DIR := .
CFLAGS := $(CFLAGS) -DRGBM_WINAMP -I$(WINAMPAPI_DIR)
SRCS := $(SRCS) rgbvis.c
TARGET := vis_sdl_rgb.dll

all: $(TARGET)

rgbm.o: greentab_winamp.h

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1 sdl
CFLAGS := $(CFLAGS) -g -fPIC -DRGBM_AUDACIOUS \
		  $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
LIBS = $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm -lfftw3
SRCS := $(SRCS) aud_rgb.c
TARGET := aud_sdl_rgb.so

.PHONY : install uninstall all

all: $(TARGET)

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

rgbm.o: greentab_audacious.h freqadj_audacious.h

endif

CFLAGS := $(CFLAGS)

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LIBS) -o $@

.PHONY : clean veryclean
clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak

veryclean: clean
	rm -f freqadj_audacious.h greentab_audacious.h greentab_winamp.h

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h rgbm.h Makefile

aud_rgb.o: aud_rgb.c rgbm.h Makefile

rgbm.o: rgbm.c rgbm.h Makefile

sdl_display.o: sdl_display.c display.h

freqadj_audacious.h: makefreqadj.m
	octave -q $^

greentab_audacious.h: makeramps.rb
	ruby makeramps.rb 144 1 512 > $@

greentab_winamp.h: makeramps.rb
	ruby makeramps.rb 291 -1 1024 > $@
