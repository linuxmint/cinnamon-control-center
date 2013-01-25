SUBDIRS= \
	common \
	background \
	screen \
	color \
	display \
	online-accounts \
	region \
	info \
	keyboard \
	universal-access \
	datetime

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
