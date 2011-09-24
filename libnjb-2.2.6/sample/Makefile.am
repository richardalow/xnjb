bin_PROGRAMS=@CURSESPLAY@ delfile deltr dumpeax dumptime files \
	fwupgrade getfile getowner gettr getusage handshake pl play \
	playlists sendfile sendtr setowner setpbm settime tagtr tracks

cursesplay_SOURCES=cursesplay.c common.h
delfile_SOURCES=delfile.c common.h
deltr_SOURCES=deltr.c common.h
dumpeax_SOURCES=dumpeax.c common.h
dumptime_SOURCES=dumptime.c common.h
files_SOURCES=files.c common.h
fwupgrade_SOURCES=fwupgrade.c common.h
getfile_SOURCES=getfile.c common.h
getowner_SOURCES=getowner.c common.h
gettr_SOURCES=gettr.c common.h
getusage_SOURCES=getusage.c common.h
handshake_SOURCES=handshake.c common.h
pl_SOURCES=pl.c common.h
play_SOURCES=play.c common.h
playlists_SOURCES=playlists.c common.h
sendfile_SOURCES=sendfile.c common.h
sendtr_SOURCES=sendtr.c common.h
setowner_SOURCES=setowner.c common.h
setpbm_SOURCES=setpbm.c common.h
settime_SOURCES=settime.c common.h
tagtr_SOURCES=tagtr.c common.h
tracks_SOURCES=tracks.c common.h

EXTRA_PROGRAMS=cursesplay

AM_CFLAGS=-I$(top_builddir)/src
LDADD=../src/libnjb.la
cursesplay_LDADD=$(LDADD) -lcurses
fwupgrade_LDADD=$(LDADD) $(SAMPLE_LDADD)
