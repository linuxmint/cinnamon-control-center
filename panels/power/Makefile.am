cappletname = power

SUBDIRS = icons

INCLUDES = 						\
	$(PANEL_CFLAGS)					\
	$(POWER_PANEL_CFLAGS)				\
	-DCINNAMONCC_UI_DIR="\"$(uidir)\""			\
	-DCINNAMONLOCALEDIR="\"$(datadir)/locale\""	\
	-DCINNAMONCC_DATA_DIR="\"$(pkgdatadir)\""		\
	-I$(srcdir)/../../shell/			\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libpower.la

libpower_la_SOURCES =		\
	power-module.c		\
	cc-power-panel.c	\
	cc-power-panel.h

libpower_la_LIBADD = $(PANEL_LIBS) $(POWER_PANEL_LIBS)
libpower_la_LDFLAGS = $(PANEL_LDFLAGS)

uidir = $(pkgdatadir)/ui
dist_ui_DATA = power.ui

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = cinnamon-power-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(desktop_in_files) $(desktop_DATA)

-include $(top_srcdir)/git.mk
