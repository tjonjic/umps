XCPP_FLAGS = -I$(top_srcdir)/src
CCASFLAGS="-KPIC"

umps_datadir = $(datadir)/umps2
umps_includedir = $(includedir)/umps2/umps
umps_libdir = $(libdir)/umps2

EXTRA_DIST = \
	coreboot.S tapeboot.S			\
	exec.S					\
	crti.template.S crti.S crtso.S		\
	elf32btsmip.h.umpsaout.x		\
	elf32btsmip.h.umpscore.x		\
	elf32ltsmip.h.umpsaout.x		\
	elf32ltsmip.h.umpscore.x		\
	libumps.S				\
	regdef.h				\
	h/bios_srv.h				\
	bios_defs.h

CLEANFILES = exec coreboot tapeboot *.o *.rom.umps 

umps_data_DATA = \
	exec.rom.umps coreboot.rom.umps tapeboot.rom.umps	\
	$(LDCORESCRIPT) $(LDAOUTSCRIPT)

umps_lib_DATA = crtso.o crti.o libumps.o

umps_include_HEADERS = e/libumps.e h/types.h

#all: exec.rom.umps coreboot.rom.umps tapeboot.rom.umps libumps.o crtso.o crti.template.o crti.o

# generic aout startup function 
.S.o:
	$(XCGCC) -c $(XCPP_FLAGS) -Wa,$(CCASFLAGS) $<

exec: exec.S
	$(XCGCC) -c $(XCPP_FLAGS) -Wa,$(CCASFLAGS) $< -o $@

coreboot: coreboot.S
	$(XCGCC) -c $(XCPP_FLAGS) -Wa,$(CCASFLAGS) $< -o $@

tapeboot: tapeboot.S
	$(XCGCC) -c $(XCPP_FLAGS) -Wa,$(CCASFLAGS) $< -o $@

exec.rom.umps: exec
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

tapeboot.rom.umps: tapeboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

coreboot.rom.umps: coreboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<
