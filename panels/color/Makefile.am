SUBDIRS = icons

cappletname = color

AM_CPPFLAGS = 						\
	$(WARN_CFLAGS)					\
	$(PANEL_CFLAGS)					\
	$(COLOR_PANEL_CFLAGS)				\
	-DCINNAMONCC_UI_DIR="\"$(uidir)\""			\
	-DLOCALE_DIR="\"$(datadir)/locale\""	\
	-DBINDIR="\"$(bindir)\""			\
	-DCINNAMONCC_DATA_DIR="\"$(pkgdatadir)\""		\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libcolor.la

libcolor_la_SOURCES =		\
	color-module.c		\
	cc-color-panel.c	\
	cc-color-panel.h

libcolor_la_LIBADD = $(PANEL_LIBS) $(COLOR_PANEL_LIBS)
libcolor_la_LDFLAGS = $(WARN_LDFLAGS) $(PANEL_LDFLAGS)

uidir = $(pkgdatadir)/ui
dist_ui_DATA = color.ui

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = cinnamon-color-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(desktop_in_files) $(desktop_DATA)

-include $(top_srcdir)/git.mk
