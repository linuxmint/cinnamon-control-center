SUBDIRS = data

# This is used in PANEL_CFLAGS
cappletname = sound-nua
NULL =

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libsoundnua.la

AM_CPPFLAGS =						\
	$(PANEL_CFLAGS) 				\
	$(SOUND_PANEL_CFLAGS)				\
	-DLOCALE_DIR=\""$(datadir)/locale"\"		\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DGLADEDIR=\""$(pkgdatadir)"\"			\
        -DSOUND_DATA_DIR="\"$(datadir)/sounds\""	\
        -DSOUND_SET_DIR="\"$(pkgdatadir)/sounds\""	\
	-DICON_DATA_DIR="\"$(pkgdatadir)/icons\""	\
	-I../sound					\
	$(NULL)

libsoundnua_la_LIBADD =				\
	-lm					\
	../sound/libcinnamonvolumecontrol.la	\
	$(PANEL_LIBS)				\
	$(SOUND_PANEL_LIBS)			\
	$(NULL)

libsoundnua_la_LDFLAGS =			\
	$(PANEL_LDFLAGS)			\
	$(NULL)

libsoundnua_la_SOURCES =			\
	gvc-balance-bar.h			\
	gvc-balance-bar.c			\
	gvc-mixer-dialog.h			\
	gvc-mixer-dialog.c			\
	../sound/gvc-level-bar.h		\
	../sound/gvc-level-bar.c		\
	../sound/gvc-combo-box.h		\
	../sound/gvc-combo-box.c		\
	../sound/gvc-speaker-test.h		\
	../sound/gvc-speaker-test.c		\
	cc-sound-panel.c			\
	cc-sound-panel.h			\
	$(NULL)

BUILT_SOURCES =				\
	$(NULL)

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

-include $(top_srcdir)/git.mk
