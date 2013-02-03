# This is used in PANEL_CFLAGS
cappletname = region

INCLUDES =						\
	$(PANEL_CFLAGS)					\
	$(REGION_PANEL_CFLAGS)				\
	-DCINNAMONLOCALEDIR="\"$(datadir)/locale\""	\
	-DCINNAMONCC_DATA_DIR="\"$(pkgdatadir)\""		\
	-DCINNAMONCC_UI_DIR="\"$(uidir)\""			\
	-I$(srcdir)/../common/				\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libregion.la

libregion_la_SOURCES =	\
	region-module.c \
	cc-region-panel.c \
	cc-region-panel.h \
	cinnamon-region-panel-formats.c \
	cinnamon-region-panel-formats.h \
	cinnamon-region-panel-lang.c \
	cinnamon-region-panel-lang.h \
	cinnamon-region-panel-system.c \
	cinnamon-region-panel-system.h \
	cinnamon-region-panel-xkb.c \
	cinnamon-region-panel-xkblt.c \
	cinnamon-region-panel-xkbltadd.c \
	cinnamon-region-panel-xkbot.c \
	cinnamon-region-panel-xkbpv.c \
	cinnamon-region-panel-xkb.h

libregion_la_LIBADD = $(PANEL_LIBS) $(REGION_PANEL_LIBS) $(builddir)/../common/liblanguage.la

libregion_la_LDFLAGS = $(PANEL_LDFLAGS)

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
ui_DATA = \
	cinnamon-region-panel.ui \
	cinnamon-region-panel-layout-chooser.ui \
	cinnamon-region-panel-options-dialog.ui

desktopdir = $(datadir)/applications
Desktop_in_files = cinnamon-region-panel.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(Desktop_in_files) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA)


-include $(top_srcdir)/git.mk
