#
# Gaim Extended Preferences Plugin Main Makefile
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

EP_SRC =		./src

BIN_DIST =	ChangeLog \
			INSTALL-win32.txt \
			LICENSE

SOURCE_DIST = 	INSTALL \
				Makefile \
				VERSION

# This is a little on the hacky side, but it should probably guess properly
# if the user is compiling using mingw gcc....
MF_EXT = $(shell which gcc | sed "s+.*/minGW/.*+.mingw+" | grep -e mingw)

EP_VERSION = $(shell cat ./VERSION)

all:
	$(MAKE) -C $(EP_SRC) -f Makefile$(MF_EXT)

install: all
	$(MAKE) -C $(EP_SRC) -f Makefile$(MF_EXT) install

zip: all
	cp src/extendedprefs.dll ./
	zip -qD extendedprefs-$(EP_VERSION).zip $(BIN_DIST) extendedprefs.dll
	rm -f extendedprefs.dll

dist:
	mkdir -p extendedprefs-$(EP_VERSION)
	$(MAKE) -C $(EP_SRC) -f Makefile dist
	cp $(BIN_DIST) $(SOURCE_DIST) extendedprefs-$(EP_VERSION)
	tar -czf extendedprefs-$(EP_VERSION).tar.gz extendedprefs-$(EP_VERSION)
	rm -rf extendedprefs-$(EP_VERSION)

clean:
	$(MAKE) -C $(EP_SRC) -f Makefile$(MF_EXT) clean
	rm -f extendedprefs-$(EP_VERSION).zip
	rm -f extendedprefs-$(EP_VERSION).tar.gz