# This is used in PANEL_CFLAGS
cappletname = common

noinst_LTLIBRARIES = liblanguage.la
noinst_PROGRAMS = list-languages

AM_CPPFLAGS =						\
	$(PANEL_CFLAGS)					\
	$(LIBLANGUAGE_CFLAGS)				\
	-DDATADIR=\""$(datadir)"\"			\
	-DUIDIR=\""$(pkgdatadir)/ui"\"			\
	-DLIBLOCALEDIR=\""$(prefix)/lib/locale"\"       \
	-DCINNAMONLOCALEDIR=\""$(datadir)/locale"\"        \
	-DUM_PIXMAP_DIR=\""$(pkgdatadir)/pixmaps"\"

liblanguage_la_SOURCES =		\
	gdm-languages.h 		\
	gdm-languages.c 		\
	locarchive.h			\
	cc-common-language.c		\
	cc-common-language.h		\
	cc-language-chooser.c		\
	cc-language-chooser.h

liblanguage_la_LIBADD = 		\
	$(PANEL_LIBS)			    \
	$(LIBLANGUAGE_LIBS)

liblanguage_la_LDFLAGS = $(PANEL_LDFLAGS)

list_languages_SOURCES = list-languages.c
list_languages_LDADD = liblanguage.la
list_languages_CFLAGS = $(LIBLANGUAGE_CFLAGS)

uidir = $(pkgdatadir)/ui

dist_ui_DATA = \
	language-chooser.ui

rulesdir = $(datadir)/polkit-1/rules.d/
rules_DATA = cinnamon-control-center.rules

EXTRA_DIST = $(rules_DATA)

-include $(top_srcdir)/git.mk
