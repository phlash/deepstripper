RELEASE=0.1
LIN_FLAGS=$(shell pkg-config --cflags gtk+-2.0) -g
LIN_LIBS=$(shell pkg-config --libs gtk+-2.0) -g

WIN_FLAGS=-mms-bitfields
WIN_LIBS=$(shell PKG_CONFIG_PATH=/usr/i586-mingw32msvc/lib/pkgconfig pkg-config --libs-only-l gtk+-win32-2.0)

LINOUT=linux
WINOUT=win32

all: lin win
lin: $(LINOUT) $(LINOUT)/deepstripper-gtk
win: $(WINOUT) $(WINOUT)/deepstripper.exe

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

$(WINOUT)/deepstripper.exe: CC=i586-mingw32msvc-gcc
$(WINOUT)/deepstripper.exe: CFLAGS=$(WIN_FLAGS) -DRELEASE=\"$(RELEASE)\"
$(WINOUT)/deepstripper.exe: LIBS=$(WIN_LIBS)

clean:
	rm -rf $(LINOUT) $(WINOUT)

OBJS=deepstripper-gtk.o \
	akaiosdisk.o \
	akaiosproject.o \
	akaiosscene.o \
	akaiosutil.o

$(LINOUT)/deepstripper-gtk: $(OBJS:%=$(LINOUT)/%)
	$(CC) -o $@ $^ $(LIBS)

$(WINOUT)/deepstripper.exe: $(OBJS:%=$(WINOUT)/%)
	$(CC) -o $@ -Wl,-subsystem,windows $^ $(LIBS)

$(LINOUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(WINOUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
