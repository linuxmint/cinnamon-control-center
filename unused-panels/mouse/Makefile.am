# This is used in PANEL_CFLAGS
cappletname = mouse

INCLUDES = 						\
	$(PANEL_CFLAGS)					\
	$(MOUSE_PANEL_CFLAGS)				\
	-DCINNAMONLOCALEDIR="\"$(datadir)/locale\""	\
	-DCINNAMONCC_DATA_DIR="\"$(pkgdatadir)\""		\
	-DCINNAMONCC_UI_DIR="\"$(uidir)\""			\
	$(NULL)


ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libmouse-properties.la

libmouse_properties_la_SOURCES =	\
	mouse-module.c			\
	cc-mouse-panel.c		\
	cc-mouse-panel.h		\
	cinnamon-mouse-properties.c	\
	cinnamon-mouse-properties.h	\
	gsd-input-helper.c		\
	gsd-input-helper.h		\
	cinnamon-mouse-test.c		\
	cinnamon-mouse-test.h

libmouse_properties_la_LIBADD = $(PANEL_LIBS) $(MOUSE_PANEL_LIBS)
libmouse_properties_la_LDFLAGS = $(PANEL_LDFLAGS)

test_gnome_mouse_test_SOURCES =			\
	cinnamon-mouse-test.c		\
	cinnamon-mouse-test.h		\
	test-cinnamon-mouse-test.c

noinst_PROGRAMS = test-cinnamon-mouse-test

test_gnome_mouse_test_CPPFLAGS = $(INCLUDES)
test_gnome_mouse_test_LDADD = $(PANEL_LIBS) $(MOUSE_PANEL_LIBS)

@INTLTOOL_DESKTOP_RULE@

uidir   = $(pkgdatadir)/ui
ui_DATA =				\
	cinnamon-mouse-properties.ui	\
	cinnamon-mouse-test.ui		\
	scroll-test-gegl.svg		\
	scroll-test.svg

desktopdir = $(datadir)/applications
Desktop_in_files = cinnamon-mouse-panel.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

INPUTDIR=$(top_srcdir)/../gnome-settings-daemon/plugins/common/
INPUTFILES=gsd-input-helper.h gsd-input-helper.c
update-from-gsd:
	FILES="$(INPUTFILES)" DIR="$(INPUTDIR)" $(top_srcdir)/update-from-gsd.sh && changed=true ; \
	git commit -m "mouse: Update from gnome-settings-daemon" $(INPUTFILES)

CLEANFILES = $(Desktop_in_files) $(desktop_DATA)
EXTRA_DIST = $(ui_DATA)

-include $(top_srcdir)/git.mk
