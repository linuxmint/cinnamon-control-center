NULL =

enabled_panels = \
	common \
	display \
	region \
	datetime

disabled_panels = $(NULL)

if BUILD_NETWORK
enabled_panels += network
else
disabled_panels += network
endif

if BUILD_COLOR
enabled_panels += color
else
disabled_panels += color
endif

if BUILD_WACOM
enabled_panels += wacom
else
disabled_panels += wacom
endif

if BUILD_ONLINE_ACCOUNTS
enabled_panels += online-accounts
else
disabled_panels += online-accounts
endif

SUBDIRS = common $(enabled_panels)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_panels)

-include $(top_srcdir)/git.mk
