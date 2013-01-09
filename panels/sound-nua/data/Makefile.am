NULL =

@INTLTOOL_DESKTOP_RULE@

appsdir = $(datadir)/applications
apps_in_files = gnome-sound-nua-panel.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

EXTRA_DIST =					\
	gnome-sound-nua-panel.desktop.in.in		\
	$(NULL)

CLEANFILES =					\
	gnome-sound-nua-panel.desktop		\
	$(NULL)

-include $(top_srcdir)/git.mk
