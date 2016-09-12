# This is used in PANEL_CFLAGS
cappletname = wacom

SUBDIRS = calibrator

AM_CPPFLAGS = 						\
	$(PANEL_CFLAGS)					\
	$(WACOM_PANEL_CFLAGS)				\
	-I$(srcdir)/calibrator				\
	-DCINNAMONLOCALEDIR="\"$(datadir)/locale\""	\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libwacom-properties.la

BUILT_SOURCES =			\
	cc-wacom-resources.c	\
	cc-wacom-resources.h

libwacom_properties_la_SOURCES =	\
	$(BUILT_SOURCES)		\
	wacom-module.c			\
	cc-wacom-panel.c		\
	cc-wacom-panel.h		\
	cc-wacom-page.c			\
	cc-wacom-page.h			\
	cc-wacom-stylus-page.c		\
	cc-wacom-stylus-page.h		\
	csd-wacom-device.c		\
	csd-wacom-device.h		\
	csd-input-helper.c		\
	csd-input-helper.h		\
	cc-wacom-nav-button.c		\
	cc-wacom-nav-button.h		\
	cc-wacom-mapping-panel.c	\
	cc-wacom-mapping-panel.h	\
	csd-enums.h

libwacom_properties_la_LIBADD = $(PANEL_LIBS) $(WACOM_PANEL_LIBS) $(builddir)/calibrator/libwacom-calibrator.la
libwacom_properties_la_LDFLAGS = $(WARN_LDFLAGS) $(PANEL_LDFLAGS)

noinst_PROGRAMS = test-wacom

test_wacom_SOURCES =			\
	$(BUILT_SOURCES)		\
	test-wacom.c			\
	cc-wacom-page.c			\
	cc-wacom-page.h			\
	cc-wacom-stylus-page.c		\
	cc-wacom-stylus-page.h		\
	cc-wacom-nav-button.c		\
	cc-wacom-nav-button.h		\
	cc-wacom-mapping-panel.c	\
	cc-wacom-mapping-panel.h	\
	csd-wacom-device.c		\
	csd-wacom-device.h		\
	csd-input-helper.c		\
	csd-input-helper.h		\
	csd-enums.h

test_wacom_CFLAGS = $(AM_CFLAGS)
test_wacom_LDADD = $(PANEL_LIBS) $(WACOM_PANEL_LIBS) $(builddir)/calibrator/libwacom-calibrator.la

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/wacom.gresource.xml)
cc-wacom-resources.c: wacom.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name cc_wacom $<
cc-wacom-resources.h: wacom.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name cc_wacom $<

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = cinnamon-wacom-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

WACOMDIR=$(top_srcdir)/../cinnamon-settings-daemon/plugins/wacom/
WACOMFILES=csd-wacom-device.c csd-wacom-device.h
COMMONDIR=$(top_srcdir)/../cinnamon-settings-daemon/plugins/common/
COMMONFILES=csd-input-helper.c csd-input-helper.h
update-from-gsd:
	FILES="$(WACOMFILES)" DIR="$(WACOMDIR)" $(top_srcdir)/update-from-gsd.sh && changed=true ; \
	FILES="$(COMMONFILES)" DIR="$(COMMONDIR)" $(top_srcdir)/update-from-gsd.sh && changed=true ; \
	git commit -m "wacom: Update from cinnamon-settings-daemon" $(WACOMFILES) $(COMMONFILES)

CLEANFILES = $(Desktop_in_files) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST = $(resource_files) wacom.gresource.xml

-include $(top_srcdir)/git.mk
