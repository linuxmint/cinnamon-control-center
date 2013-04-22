SUBDIRS= \
	common \
	screen \
	color \
	display \
	region \
	universal-access \
	user-accounts  \
	power \
	sound \
	sound-nua

if BUILD_NETWORK
SUBDIRS += network
endif

if BUILD_BLUETOOTH
SUBDIRS += bluetooth
endif

-include $(top_srcdir)/git.mk
