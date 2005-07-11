EXTRA_DIST =                 \
	ChangeLog            \
	INSTALL              \
	INSTALL-win32.txt    \
	LICENSE              \
	Makefile.mingw       \
	VERSION              \
	src/Makefile.mingw

SUBDIRS = src

BUILT_SOURCES = extprefs_config.h

$(OBJECTS): $(BUILT_SOURCES)

extprefs_config.h: pre_config.h
	$(sedpath) 's/#define PACKAGE/#define EXTPREFS_PACKAGE/g' pre_config.h > $@
