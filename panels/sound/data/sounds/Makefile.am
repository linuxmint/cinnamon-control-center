NULL =

sounddir = $(datadir)/sounds/cinnamon/default/alerts

sound_DATA =			\
	bark.ogg		\
	drip.ogg		\
	glass.ogg		\
	sonar.ogg		\
	$(NULL)

metadata_in_files = cinnamon-sounds-default.xml.in
metadatadir = $(pkgdatadir)/sounds
metadata_DATA = $(metadata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

noinst_DATA = cinnamon-sounds-default.xml.in
CLEANFILES = cinnamon-sounds-default.xml cinnamon-sounds-default.xml.in

EXTRA_DIST = $(sound_DATA) cinnamon-sounds-default.xml.in.in

cinnamon-sounds-default.xml.in: cinnamon-sounds-default.xml.in.in Makefile
	$(AM_V_GEN)sed -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/cinnamon-sounds-default.xml.in.in > cinnamon-sounds-default.xml.in.tmp \
	&& mv cinnamon-sounds-default.xml.in.tmp cinnamon-sounds-default.xml.in

MAINTAINERCLEANFILES = \
	Makefile.in

-include $(top_srcdir)/git.mk
