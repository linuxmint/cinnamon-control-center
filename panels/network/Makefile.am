SUBDIRS = wireless-security connection-editor

cappletname = network

AM_CPPFLAGS = 						\
	$(PANEL_CFLAGS)					\
	$(NETWORK_PANEL_CFLAGS)				\
	$(NETWORK_MANAGER_CFLAGS)			\
	-DLOCALE_DIR="\"$(datadir)/locale\""	\
	-I$(srcdir)/wireless-security			\
	$(NULL)

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libnetwork.la

BUILT_SOURCES =			\
	cc-network-resources.c	\
	cc-network-resources.h

libnetwork_la_SOURCES =					\
	$(BUILT_SOURCES)				\
	network-module.c				\
	panel-common.c					\
	panel-common.h					\
	net-object.c					\
	net-object.h					\
	net-device.c					\
	net-device.h					\
	net-device-wifi.c				\
	net-device-wifi.h				\
	net-device-simple.c				\
	net-device-simple.h				\
	net-device-ethernet.c				\
	net-device-ethernet.h				\
	net-device-bond.c				\
	net-device-bond.h				\
	net-device-bridge.c				\
	net-device-bridge.h				\
	net-virtual-device.c				\
	net-virtual-device.h				\
	net-vpn.c					\
	net-vpn.h					\
	net-proxy.c					\
	net-proxy.h					\
	network-dialogs.c				\
	network-dialogs.h				\
	list-box-helper.h			\
	list-box-helper.c			\
	cc-network-panel.c				\
	cc-network-panel.h

if BUILD_MODEM
libnetwork_la_SOURCES +=			\
	net-device-mobile.c				\
	net-device-mobile.h
NETWORK_PANEL_CFLAGS += -DBUILD_MODEM
AM_CPPFLAGS += $(MODEM_MANAGER_CFLAGS)
endif

TEAM_FILES = net-device-team.c net-device-team.h
EXTRA_DIST = $(TEAM_FILES)
if HAVE_NM_UNSTABLE
libnetwork_la_SOURCES += $(TEAM_FILES)
endif

if BUILD_MODEM
libnetwork_la_LIBADD = $(PANEL_LIBS) $(NETWORK_PANEL_LIBS) $(NETWORK_MANAGER_LIBS) $(MODEM_MANAGER_LIBS) $(builddir)/connection-editor/libconnection-editor.la
else
libnetwork_la_LIBADD = $(PANEL_LIBS) $(NETWORK_PANEL_LIBS) $(NETWORK_MANAGER_LIBS) $(builddir)/connection-editor/libconnection-editor.la
endif

libnetwork_la_LDFLAGS = $(WARN_LDFLAGS) $(PANEL_LDFLAGS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/network.gresource.xml)
cc-network-resources.c: network.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name cc_network $<
cc-network-resources.h: network.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name cc_network $<

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = cinnamon-network-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(desktop_in_files) $(desktop_DATA) $(BUILT_SOURCES)
EXTRA_DIST += $(resource_files) network.gresource.xml

-include $(top_srcdir)/git.mk
