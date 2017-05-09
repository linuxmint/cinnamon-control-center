AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	$(SHELL_CFLAGS)

bin_PROGRAMS = cinnamon-control-center

MARSHAL_FILES = cc-shell-marshal.c cc-shell-marshal.h
BUILT_SOURCES = $(MARSHAL_FILES)

cc-shell-marshal.h: cc-shell-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=cc_shell_marshal $< --header > $@

cc-shell-marshal.c: cc-shell-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=cc_shell_marshal $< --body --header > $@

cinnamon_control_center_SOURCES =			\
	control-center.c			\
	cc-shell-log.c				\
	cc-shell-log.h				\
	cinnamon-control-center.c			\
	cinnamon-control-center.h			\
	cc-shell-category-view.c		\
	cc-shell-category-view.h		\
	cc-shell-item-view.c			\
	cc-shell-item-view.h			\
	cc-shell-model.c			\
	cc-shell-model.h			\
	cc-shell-nav-bar.c			\
	cc-shell-nav-bar.h			\
	$(MARSHAL_FILES)

cinnamon_control_center_LDADD =			\
	libcinnamon-control-center.la		\
	$(SHELL_LIBS)

cinnamon_control_center_LDFLAGS = $(WARN_LDFLAGS) -export-dynamic

lib_LTLIBRARIES = libcinnamon-control-center.la

libcinnamon_control_center_include_HEADERS =      \
	cc-panel.h                              \
	cc-shell.h                              \
	cc-editable-entry.h			\
	$(NULL)

libcinnamon_control_center_la_SOURCES =		\
	cc-panel.c				\
	cc-panel.h				\
	cc-shell.c				\
	cc-shell.h				\
	cc-editable-entry.c			\
	cc-editable-entry.h			\
	hostname-helper.h			\
	hostname-helper.c			\
	list-box-helper.h			\
	list-box-helper.c			\
	$(NULL)

libcinnamon_control_center_la_LDFLAGS =		\
	$(WARN_LDFLAGS)				\
	-no-undefined				\
	-version-info $(LIBCINNAMONCONTROLCENTER_CURRENT):$(LIBCINNAMONCONTROLCENTER_REVISION):$(LIBCINNAMONCONTROLCENTER_AGE) \
	$(NULL)

libcinnamon_control_center_la_LIBADD = 		\
	$(LIBCINNAMON_CONTROL_CENTER_LIBS)		\
	$(NULL)

libcinnamon_control_center_la_LIBTOOLFLAGS = --tag=disable-static

libcinnamon_control_center_includedir = $(includedir)/cinnamon-control-center-1/libcinnamon-control-center

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libcinnamon-control-center.pc

AM_CPPFLAGS +=							\
	-DCINNAMONLOCALEDIR="\"$(datadir)/locale\""		\
	-DUIDIR="\"$(uidir)\""					\
	-DMENUDIR="\"$(menudir)\""				\
	-DPANELS_DIR="\"$(PANELS_DIR)\""

menudir = $(sysconfdir)/xdg/menus
menu_DATA = cinnamoncc.menu

cinnamoncc.menu: cinnamoncc.menu.in
	$(AM_V_GEN) cat $< | sed 's,@applicationsdir@,$(datadir)/applications/,' > $@

uidir = $(pkgdatadir)/ui
ui_DATA = shell.ui

sysdir = $(datadir)/applications
sys_in_files = cinnamon-control-center.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

directorydir = $(datadir)/desktop-directories
directory_in_files = cinnamoncc.directory.in
directory_DATA = $(directory_in_files:.directory.in=.directory)
@INTLTOOL_DIRECTORY_RULE@

EXTRA_DIST =					\
	$(ui_DATA)				\
	cinnamon-control-center.desktop.in.in	\
	cinnamoncc.directory.in			\
	cinnamoncc.menu.in			\
	libcinnamon-control-center.pc.in	\
	cc-shell-marshal.list

DISTCLEANFILES = cinnamon-control-center.desktop cinnamon-control-center.desktop.in cinnamoncc.directory cinnamoncc.menu

-include $(top_srcdir)/git.mk
