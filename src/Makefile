#
# Gaim Extended Preferences Plugin Source Makefile (Unix)
#
# Copyright 2004 Kevin Stange <extprefs@simguy.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

CC := gcc
LIBTOOL := libtool

ifeq ($(PREFIX),)
  LIB_INSTALL_DIR = $(HOME)/.gaim/plugins
else
  LIB_INSTALL_DIR = $(PREFIX)/lib/gaim
endif

EP_TOP =		../

EP = extendedprefs
EP_VERSION = $(shell cat ${EP_TOP}/VERSION)

GAIM_CFLAGS  = $(shell pkg-config gaim --cflags)
GTK_CFLAGS   = $(shell pkg-config gtk+-2.0 --cflags)
GAIM_LIBS    = $(shell pkg-config gaim --libs)
GTK_LIBS     = $(shell pkg-config gtk+-2.0 --libs)
GAIM_LIBDIR  = $(shell pkg-config --variable=libdir gaim)/gaim

EP_SRC =	extendedprefs.c

SOURCE_DIST = 	Makefile \
				Makefile.mingw

DEFINES = -DEP_VERSION=\"$(EP_VERSION)\"

.PHONY: all clean install

all: $(EP).so

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	cp $(EP).so $(LIB_INSTALL_DIR)

$(EP).so: $(EP).c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $< -o tmp$@.lo $(GAIM_CFLAGS) $(GTK_CFLAGS) -DHAVE_CONFIG_H $(DEFINES)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(GAIM_LIBDIR) tmp$@.lo $(GAIM_LIBS) $(GTK_LIBS) -module -avoid-version
	rm -f tmp$@.lo tmp$@.o libtmp$@.la
	cp .libs/libtmp$@.so* $@

dist:
	mkdir -p $(EP_TOP)extendedprefs-$(EP_VERSION)/src
	cp $(SOURCE_DIST) $(EP_SRC) $(EP_TOP)extendedprefs-$(EP_VERSION)/src

clean:
	rm -rf *.o *.so *.la .libs
