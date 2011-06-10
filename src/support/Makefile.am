XCPP_FLAGS = -I$(top_srcdir)/src

BIOS_CFLAGS = -mno-abicalls -fno-pic -ffreestanding

CRT_CFLAGS_PIC = -ffreestanding -DABICALLS -Wa,-KPIC
CRT_CFLAGS_NOPIC = -ffreestanding -mno-abicalls -fno-pic

umps_datadir = $(datadir)/umps2
umps_includedir = $(includedir)/umps2/umps
umps_libdir = $(libdir)/umps2

EXTRA_DIST = \
	coreboot.S				\
	tapeboot.S				\
	exec.S					\
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

CLEANFILES = exec coreboot tapeboot *.o *.rom.umps 

umps_data_DATA = \
	exec.rom.umps				\
	coreboot.rom.umps			\
	tapeboot.rom.umps			\
	$(LDCORESCRIPT)				\
	$(LDAOUTSCRIPT)				\
	umpsaout.ldscript			\
	umpscore.ldscript			\
	crtso.S					\
	libumps.S

umps_lib_DATA = \
	crtso.o					\
	crti.o					\
	libumps.o

umps_include_HEADERS = e/libumps.e h/types.h

crtso.o : crtso_pic.S
	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) -o $@ $<

crti.o : crti.S
	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) $<

libumps.o : libumps.S
	$(XCGCC) -c $(XCPP_FLAGS) $(CRT_CFLAGS_PIC) $<

exec: exec.S
	$(XCGCC) -c $(XCPP_FLAGS) $(BIOS_CFLAGS) $< -o $@

coreboot: coreboot.S
	$(XCGCC) -c $(XCPP_FLAGS) $(BIOS_CFLAGS) $< -o $@

tapeboot: tapeboot.S
	$(XCGCC) -c $(XCPP_FLAGS) $(BIOS_CFLAGS) $< -o $@

exec.rom.umps: exec
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

tapeboot.rom.umps: tapeboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

coreboot.rom.umps: coreboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<
