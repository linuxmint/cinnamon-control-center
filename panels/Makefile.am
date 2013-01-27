SUBDIRS= \
	common \
	screen \
	color \
	display \
	online-accounts \
	region \
	info \
	keyboard \
	universal-access \
	datetime \
	user-accounts  \
	power \
	sound \
	sound-nua

if BUILD_PRINTERS
SUBDIRS += printers
endif

if BUILD_NETWORK
SUBDIRS += network
endif

if BUILD_BLUETOOTH
SUBDIRS += bluetooth
endif

-include $(top_srcdir)/git.mk
