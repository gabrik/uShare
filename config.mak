# Automatically generated by configure - do not modify!
VERSION=1.1a
PREFIX=/usr/local
prefix=$(DESTDIR)$(PREFIX)
bindir=$(DESTDIR)${PREFIX}/bin
sysconfdir=$(DESTDIR)${PREFIX}/etc
localedir=$(DESTDIR)${PREFIX}/share/locale
MAKE=make
CC=gcc
LN=ln
STRIP=echo ignoring strip
INSTALLSTRIP=
EXTRALIBS= -lixml -lthreadutil -lpthread -lupnp -pthread -ldlna -L/usr/local/lib -lavformat -lavcodec -lavformat-ffmpeg -lavcodec-ffmpeg -lulfius -ljansson -lyder -lorcania -lcurl -lglib-2.0
OPTFLAGS= -I.. -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE -g3 -DHAVE_DEBUG -DHAVE_LOCALE_H -DHAVE_SETLOCALE -DHAVE_IFADDRS_H -DHAVE_LANGINFO_H -DHAVE_LANGINFO_CODESET -DHAVE_ICONV -pthread -I/usr/include/upnp -DHAVE_DLNA -I/usr/local/include -I/usr/include/x86_64-linux-gnu -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LDFLAGS=
INSTALL=/usr/bin/install -c
DEBUG=yes
