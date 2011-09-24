SUBDIRS=src sample doc

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libnjb.pc
EXTRA_DIST=libnjb.pc \
nomad.usermap \
nomad.rules \
nomadjukebox \
libnjb.fdi \
ChangeLog-old \
FAQ LICENSE HACKING \
windows

dist-hook:
	rm -rf `find $(distdir)/windows -name CVS`

if HOTPLUGENABLE
hotplugdir=@HOTPLUGPATH@/usb
hotplug_DATA=nomad.usermap nomadjukebox
endif
