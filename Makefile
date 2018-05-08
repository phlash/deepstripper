RELEASE=0.8
BITS=$(shell getconf LONG_BIT)
ifeq ($(BITS),64)
ARCH=amd64
else
ARCH=i386
endif

LIN_FLAGS=$(shell pkg-config --cflags gtk+-3.0) -g -fPIC -DGTK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
LIN_LIBS=$(shell pkg-config --libs gtk+-3.0) -g

# Always target 32-bit on windows, we only have 32-bit GTK libraries..
WIN_FLAGS=-m32 -mms-bitfields $(shell PKG_CONFIG_PATH=gtk-win32/lib/pkgconfig pkg-config --define-variable=prefix=`pwd`/gtk-win32 --cflags gtk+-win32-2.0)
WIN_LIBS=$(shell PKG_CONFIG_PATH=gtk-win32/lib/pkgconfig pkg-config --define-variable=prefix=`pwd`/gtk-win32 --libs gtk+-win32-2.0)

LINOUT=linux
LINPKG=deepstripper-gtk-$(RELEASE)
WINOUT=win32

all: lin win
lin: $(LINOUT) $(LINOUT)/deepstripper-gtk
win: $(WINOUT) $(WINOUT)/deepstripper.exe

archive:
	git archive --format=tar --prefix=deepstripper-gtk-$(RELEASE)/ HEAD |gzip -c >../deepstripper-gtk-$(RELEASE).tgz

linpkg: lin
	mkdir -p $(LINPKG)/DEBIAN
	sed -e "s/RELEASE/$(RELEASE)/" -e "s/ARCH/$(ARCH)/" control >$(LINPKG)/DEBIAN/control
	mkdir -p $(LINPKG)/usr/bin
	cp $(LINOUT)/deepstripper-gtk $(LINPKG)/usr/bin
	fakeroot dpkg-deb -b $(LINPKG) $(LINOUT)
	rm -rf $(LINPKG)

winpkg: win
	cp $(WINOUT)/deepstripper.exe .
	zip -ru $(WINOUT)/deepstripper-$(RELEASE).zip deepstripper.exe *.dll etc share
	rm deepstripper.exe

$(LINOUT):
	mkdir $(LINOUT)

$(WINOUT):
	mkdir $(WINOUT)

$(LINOUT)/deepstripper-gtk: CC=gcc
$(LINOUT)/deepstripper-gtk: CFLAGS=$(LIN_FLAGS) -DRELEASE=\"$(RELEASE)\"
$(LINOUT)/deepstripper-gtk: LIBS=$(LIN_LIBS)

$(WINOUT)/deepstripper.exe: CC=i686-w64-mingw32-gcc
$(WINOUT)/deepstripper.exe: CFLAGS=$(WIN_FLAGS) -DRELEASE=\"$(RELEASE)\"
$(WINOUT)/deepstripper.exe: LIBS=$(WIN_LIBS)

clean:
	rm -rf $(LINOUT) $(WINOUT)

OBJS=deepstripper-gtk.o \
	akaiosdisk.o \
	akaiosproject.o \
	akaiosscene.o \
	akaiosutil.o \
	diskbuffer.o

$(LINOUT)/deepstripper-gtk: $(OBJS:%=$(LINOUT)/%)
	$(CC) -o $@ $^ $(LIBS)

$(WINOUT)/deepstripper.exe: $(OBJS:%=$(WINOUT)/%)
	$(CC) -o $@ -Wl,-subsystem,windows $^ $(LIBS)

$(LINOUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(WINOUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
