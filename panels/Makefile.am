NULL =

enabled_panels = \
	common \
	screen \
	color \
	display \
	region \
	universal-access \
	power \
	sound \
	sound-nua \
	network \
	datetime

disabled_panels = $(NULL)

if BUILD_WACOM
enabled_panels += wacom
else
disabled_panels += wacom
endif

SUBDIRS = common $(enabled_panels)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_panels)

-include $(top_srcdir)/git.mk
