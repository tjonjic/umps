umps_datadir = $(datadir)/umps2

umps_data_DATA = \
	exec.rom.umps		\
	coreboot.rom.umps	\
	tapeboot.rom.umps

EXTRA_DIST = \
	coreboot.S		\
	tapeboot.S		\
	exec.S

CLEANFILES = coreboot tapeboot exec *.rom.umps

BIOS_CFLAGS = -mno-abicalls -fno-pic -ffreestanding
BIOS_CPPFLAGS = -I$(top_srcdir)/src/include

coreboot.rom.umps: coreboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

tapeboot.rom.umps: tapeboot
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

exec.rom.umps: exec
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

coreboot: coreboot.S
	$(XCGCC) -c $(BIOS_CPPFLAGS) $(BIOS_CFLAGS) $< -o $@

tapeboot: tapeboot.S
	$(XCGCC) -c $(BIOS_CPPFLAGS) $(BIOS_CFLAGS) $< -o $@

exec: exec.S
	$(XCGCC) -c $(BIOS_CPPFLAGS) $(BIOS_CFLAGS) $< -o $@
