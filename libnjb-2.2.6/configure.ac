# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
AC_INIT([libnjb], [2.2.6], [libnjb-users@lists.sourceforge.net])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/base.c])
AC_CONFIG_HEADER([config.h])

# This can be overridden by the command line switch
if test "$program_prefix" = NONE; then
   program_prefix=njb-
   program_transform_name="s,^,$program_prefix,;$program_transform_name"
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
# Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
             AC_MSG_WARN([*** doxygen not found, docs will not be built])
fi

# Check for Darwin
AC_MSG_CHECKING([if the host operating system is Darwin])
case "$host" in
  *-darwin*)
    AC_MSG_RESULT([yes])
    CFLAGS="$CFLAGS -DUSE_DARWIN"
    OSFLAGS="-framework IOKit"
    ;;
  *) AC_MSG_RESULT([no]) ;;
esac
AC_SUBST(OSFLAGS)

# Check for mingw compiler platform
AC_MSG_CHECKING([For MinGW32])
case "$host" in
  *-*-mingw*)
    AC_MSG_RESULT([yes])
    mingw_compiler=yes
    ;;
  *) AC_MSG_RESULT([no]) ;;
esac
AM_CONDITIONAL(COMPILE_MINGW32, [test "$mingw_compiler" = "yes"])


# Checks for libraries.
AC_CHECK_LIB([curses], [initscr], AC_SUBST([CURSESPLAY], cursesplay))
AC_CHECK_LIB([usb], [usb_control_msg],,
	AC_MSG_ERROR([I can't find the libusb libraries on your system. You
	may need to set the LDFLAGS environment variable to include the
	search path where you have libusb installed before running
	configure (e.g. setenv LDFLAGS=-L/usr/local/lib)]), "$OSFLAGS")
AC_CHECK_LIB([z], [uncompress], AC_SUBST([SAMPLE_LDADD], [-lz]))

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([ctype.h curses.h errno.h fcntl.h getopt.h libgen.h \
	limits.h stdio.h string.h sys/stat.h sys/time.h unistd.h zlib.h])
AC_CHECK_HEADER([usb.h],,
	AC_MSG_ERROR([I can't find the libusb header file on your system.
	You may need to set the CPPFLAGS environment variable to include
	the search path where you have libusb installed before running
	configure (e.g. setenv CPPFLAGS=-I/usr/local/include)]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLKSIZE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_CHECK_FUNCS(basename memset select strdup strerror strrchr strtoul usleep)

# Check for hotplug support.
AC_ARG_ENABLE(hotplugging,
	AS_HELP_STRING([--enable-hotplugging],
		[install hotplug scripts (default=no)]),
	hotplugenable=$enableval, hotplugenable="no")
AC_ARG_WITH(hotplug,
	AS_HELP_STRING([--with-hotplug=PATH],
		[location of the hotplug scripts (PATH=/etc/hotplug)]),
	hotplugpath=$withval, hotplugpath=/etc/hotplug)
AC_ARG_WITH(hotplugowner,
	AS_HELP_STRING([--with-hotplugowner=OWNER],
		[owner of the hotplug device (OWNER=CONSOLE)]),
	hotplugowner=$withval, hotplugowner=CONSOLE)
AC_ARG_WITH(hotplugperms,
	AS_HELP_STRING([--with-hotplugperms=PERMS],
		[permission bits of the hotplug device (PERMS=0600)]),
	hotplugperms=$withval, hotplugperms=0600)

AM_CONDITIONAL([HOTPLUGENABLE], [test "$hotplugenable" = yes])
AC_SUBST(HOTPLUGPATH, "$hotplugpath")
AC_SUBST(HOTPLUGOWNER, "$hotplugowner")
AC_SUBST(HOTPLUGPERMS, "$hotplugperms")

# Switches.
# Stick in "-Werror" if you want to be more aggressive.
# (No need to use AC_SUBST on this default substituted environment variable.)
CFLAGS="$CFLAGS -Wall -Wmissing-prototypes"

# Output files
AC_CONFIG_FILES([src/libnjb.h doc/Doxyfile Makefile doc/Makefile src/Makefile
	sample/Makefile nomadjukebox hotplug.sh libnjb.pc])
AC_OUTPUT
# Add execute permissions to this script.
chmod +x hotplug.sh
