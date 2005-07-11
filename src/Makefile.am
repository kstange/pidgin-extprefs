plugindir = $(libdir)/gaim

libextprefs_la_LDFLAGS = -module -avoid-version

if PLUGINS

plugin_LTLIBRARIES = libextprefs.la

libextprefs_la_SOURCES = extendedprefs.c

endif

AM_CPPFLAGS = \
	-DVERSION=\"$EXTPREFS_VERSION\" \
	-DLIBDIR=\"$(libdir)/gaim/\" \
	-DDATADIR=\"$(datadir)\" \
	$(GTK_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(DEBUG_CFLAGS)
