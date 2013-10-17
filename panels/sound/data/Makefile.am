NULL =

SUBDIRS =			\
	icons			\
	symbolic-icons		\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@
autostartdir = $(datadir)/applications
autostart_in_files = cinnamon-sound-applet.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

appsdir = $(datadir)/applications
apps_in_files = cinnamon-sound-panel.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

EXTRA_DIST =					\
	$(autostart_in_files)			\
	cinnamon-sound-panel.desktop.in.in		\
	$(NULL)


if MAINTAINER_MODE

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

endif

CLEANFILES =					\
	cinnamon-sound-panel.desktop		\
	$(NULL)

DISTCLEANFILES =				\
	cinnamon-sound-applet.desktop		\
	$(NULL)

-include $(top_srcdir)/git.mk
