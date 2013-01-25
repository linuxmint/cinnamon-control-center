SUBDIRS = faces icons

uidir = $(pkgdatadir)/ui/user-accounts
ui_DATA = \
	account-dialog.ui	\
	password-dialog.ui	\
	photo-dialog.ui		\
	user-accounts-dialog.ui \
	account-fingerprint.ui

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
Desktop_in_files = cinnamon-user-accounts-panel.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = 					\
	cinnamon-user-accounts-panel.desktop.in.in	\
	org.freedesktop.realmd.xml		\
	$(ui_DATA)

CLEANFILES =					\
	cinnamon-user-accounts-panel.desktop	\
	$(NULL)

DISTCLEANFILES =				\
	$(NULL)

-include $(top_srcdir)/git.mk
