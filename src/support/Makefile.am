SUBDIRS = bios legacy

XCPP_FLAGS = -I$(top_srcdir)/src/include

BIOS_CFLAGS = -mno-abicalls -fno-pic -ffreestanding

CRT_CFLAGS_PIC = -ffreestanding -DABICALLS -Wa,-KPIC
CRT_CFLAGS_NOPIC = -ffreestanding -mno-abicalls -fno-pic

umps_datadir = $(datadir)/umps2
umps_includedir = $(includedir)/umps2/umps
umps_libdir = $(libdir)/umps2

EXTRA_DIST = \
	crti.S					\
	crtso.S					\
	crtso_pic.S				\
	elf32btsmip.h.umpsaout.x		\
	elf32btsmip.h.umpscore.x		\
	elf32ltsmip.h.umpsaout.x		\
	elf32ltsmip.h.umpscore.x		\
	umpsaout.ldscript			\
	umpscore.ldscript			\
	libumps.S				\
	regdef.h				\
	bios_defs.h

#CLEANFILES = exec coreboot tapeboot *.o *.rom.umps 
CLEANFILES = *.o

umps_data_DATA = \
	$(LDCORESCRIPT)				\
	$(LDAOUTSCRIPT)				\
	umpsaout.ldscript			\
	umpscore.ldscript			\
	crtso.S					\
	libumps.S

#umps_lib_DATA = \
#	crtso.o					\
#	crti.o					\
#	libumps.o

umps_include_HEADERS = e/libumps.e h/types.h

#crtso.o : crtso_pic.S
#	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) -o $@ $<

crti.o : crti.S
	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) $<

libumps.o : libumps.S
	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) $<
