SUBDIRS = bios legacy crt libumps

umps_datadir = $(datadir)/umps2

EXTRA_DIST = \
	crti.S					\
	elf32btsmip.h.umpsaout.x		\
	elf32btsmip.h.umpscore.x		\
	elf32ltsmip.h.umpsaout.x		\
	elf32ltsmip.h.umpscore.x		\
	umpsaout.ldscript			\
	umpscore.ldscript

umps_data_DATA = \
	$(LDCORESCRIPT)				\
	$(LDAOUTSCRIPT)				\
	umpsaout.ldscript			\
	umpscore.ldscript
