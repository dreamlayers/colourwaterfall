PLATFORM := $(shell uname -o)

CFLAGS := $(CFLAGS) -Wall -O -g
SRCS := rgbm.c sdl_display.c

ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc
WINAMPAPI_DIR := .
CFLAGS := $(CFLAGS) -I/usr/local/cross-tools/i686-w64-mingw32/include \
          $(shell sdl-config --cflags) \
          -DRGBM_WINAMP -DRGBM_FFT -I$(WINAMPAPI_DIR)
SRCS := $(SRCS) rgbvis.c
LDFLAGS := -static-libgcc
LIBS := $(shell sdl-config --libs)  -lpthread -lm -lfftw3
TARGET := vis_colourwaterfall.dll
STANDALONE := colourwaterfall.exe

all: $(TARGET)

rgbm.o: greentab_winamp.h

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1 sdl
CFLAGS := $(CFLAGS) -g -fPIC -DRGBM_AUDACIOUS \
		  $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
SRCS := $(SRCS) aud_rgb.c
LDFLAGS :=
LIBS := $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm -lfftw3
TARGET := aud_sdl_rgb.so
STANDALONE := colourwaterfall

.PHONY : install uninstall all

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

rgbm.o: greentab_audacious.h freqadj_audacious.h

endif

STANDALONE_SRCS = $(SRCS) portaudio.c
STANDALONE_OBJS := $(STANDALONE_SRCS:%.c=%.o)

all: $(TARGET) $(STANDALONE)

$(STANDALONE): $(STANDALONE_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LIBS) -lportaudio -o $@

OBJS := $(SRCS:%.c=%.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LDFLAGS) $(LIBS) -o $@

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
