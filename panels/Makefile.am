NULL =

enabled_panels = \
	common \
	color \
	display \
	region \
	universal-access \
	sound \
	sound-nua \
	datetime

disabled_panels = $(NULL)

if BUILD_NETWORK
enabled_panels += network
else
disabled_panels += network
endif

if BUILD_WACOM
enabled_panels += wacom
else
disabled_panels += wacom
endif

SUBDIRS = common $(enabled_panels)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_panels)

-include $(top_srcdir)/git.mk
