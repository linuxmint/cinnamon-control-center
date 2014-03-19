cappletname = screen

INCLUDES = 						\
	$(PANEL_CFLAGS)					\
	$(SCREEN_PANEL_CFLAGS)			\
	-DCINNAMONCC_UI_DIR="\"$(uidir)\""			\
	-DLOCALE_DIR="\"$(datadir)/locale\""	\
	-DCINNAMONCC_DATA_DIR="\"$(pkgdatadir)\""		\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libscreen.la

libscreen_la_SOURCES =		\
	screen-module.c		\
	cc-screen-panel.c	\
	cc-screen-panel.h

libscreen_la_LIBADD = $(PANEL_LIBS) $(SCREEN_PANEL_LIBS)
libscreen_la_LDFLAGS = $(PANEL_LDFLAGS)

uidir = $(pkgdatadir)/ui
dist_ui_DATA = screen.ui

-include $(top_srcdir)/git.mk
