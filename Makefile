PLATFORM := $(shell uname -o)

CFLAGS := $(CFLAGS) -Wall -O -g
SRCS := rgbm.c sdl_display.c

ifeq ($(PLATFORM),Cygwin)

CC := i686-w64-mingw32-gcc
PKG_PREREQ := sdl portaudio-2.0
WINAMPAPI_DIR := .
CFLAGS := $(CFLAGS) \
          $(shell i686-w64-mingw32-pkg-config --cflags $(PKG_PREREQ)) \
          -DRGBM_WINAMP -DRGBM_FFT -I$(WINAMPAPI_DIR)
STANDALONE_SRCS := $(SRCS) portaudio.c
SRCS := $(SRCS) rgbvis.c
LDFLAGS := -static
LIBS := $(shell i686-w64-mingw32-pkg-config --static --libs $(PKG_PREREQ)) \
        -lpthread -lm -lfftw3
TARGET := vis_colourwaterfall.dll
PLUGLINK := $(CC) $(CFLAGS)
STANDALONE := colourwaterfall.exe

all: $(TARGET) $(STANDALONE)

rgbm.o: greentab_winamp.h

else

PKG_PREREQ := audacious glib-2.0 dbus-glib-1 dbus-1 sdl
CFLAGS := $(CFLAGS) -g -fPIC -DRGBM_AUDACIOUS \
		  $(shell pkg-config --cflags $(PKG_PREREQ)) $(PIC)
CXXFLAGS := $(CFLAGS) -std=c++11
STANDALONE_SRCS := $(SRCS) portaudio.c
SRCS := $(SRCS) aud_rgb.cc
LDFLAGS :=
LIBS := $(shell pkg-config --libs $(PKG_PREREQ)) -lpthread -lm -lfftw3
TARGET := aud_sdl_rgb.so
PLUGLINK := $(CXX) $(CXXFLAGS)
STANDALONE := colourwaterfall

.PHONY : install uninstall all

all: $(TARGET) $(STANDALONE)

install: $(TARGET)
	cp $(TARGET) ~/.local/share/audacious/Plugins/

uninstall:
	rm ~/.local/share/audacious/Plugins/$(TARGET)

rgbm.o: greentab_audacious.h freqadj_audacious.h

endif

STANDALONE_OBJS := $(STANDALONE_SRCS:%.c=%.o)

$(STANDALONE): $(STANDALONE_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LIBS) -lportaudio -o $@

OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cc=%.o)

$(TARGET): $(OBJS)
	$(PLUGLINK) -shared -Wl,--no-undefined \
	-Wl,--exclude-libs,ALL $^ $(LDFLAGS) $(LIBS) -o $@

.PHONY : clean veryclean
clean:
	rm -f $(OBJS) $(STANDALONE_OBJS) $(TARGET) $(STANDALONE) *~ *.bak

veryclean: clean
	rm -f freqadj_audacious.h greentab_audacious.h greentab_winamp.h

rgbvis.o: rgbvis.c $(WINAMPAPI_DIR)/vis.h rgbm.h Makefile

aud_rgb.o: aud_rgb.cc rgbm.h Makefile

rgbm.o: rgbm.c rgbm.h Makefile

sdl_display.o: sdl_display.c display.h

freqadj_audacious.h: makefreqadj.m
	octave -q $^

greentab_audacious.h: makeramps.py
	python3 $^ 144 1 512 > $@

greentab_winamp.h: makeramps.py
	python3 $^ 291 -1 1024 > $@
