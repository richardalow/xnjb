lib_LTLIBRARIES=libnjb.la
libnjb_la_SOURCES=base.c ioutil.c protocol.c procedure.c byteorder.c \
	playlist.c usb_io.c njb_error.c datafile.c songid.c \
	eax.c njbtime.c protocol3.c unicode.c \
	base.h byteorder.h datafile.h defs.h eax.h ioutil.h njb_error.h \
	njbtime.h playlist.h procedure.h protocol.h protocol3.h \
	songid.h unicode.h usb_io.h
include_HEADERS=libnjb.h
EXTRA_DIST=libnjb.h.in libnjb.sym

# This is the version of the *library interface*, not the revision
# of libnjb itself. Do not change this unless you're absolutely 
# certain of what the difference is. (See the libtool manual,
# section 6.3 (http://www.gnu.org/software/libtool/manual.html)
CURRENT=6
REVISION=0
AGE=1
SOVERSION=$(CURRENT):$(REVISION):$(AGE)
libnjb_la_LDFLAGS=@LDFLAGS@ -version-info $(SOVERSION)

if COMPILE_MINGW32
noinst_DATA=libnjb.lib
libnjb_la_LDFLAGS += -export-dynamic -no-undefined -export-symbols libnjb.sym
libnjb.def: libnjb.sym
	echo "LIBRARY \"@PACKAGE@\"" > libnjb.def
	echo "DESCRIPTION \"Creative NOMAD, Zen, Dell DJ Library\"" >> libnjb.def
	echo "VERSION @VERSION@" >> libnjb.def
	echo >> libnjb.def
	echo "EXPORTS" >> libnjb.def
	cat libnjb.sym >> libnjb.def
libnjb.lib: libnjb.la libnjb.def
	lib -name:libnjb-$(LT_CURRENT_MINUS_AGE).dll -def:libnjb.def -out:$@
install-data-local: libnjb.lib libnjb.def
	$(INSTALL) libnjb.def $(DESTDIR)$(libdir)
	$(INSTALL) libnjb.lib $(DESTDIR)$(libdir)
endif
