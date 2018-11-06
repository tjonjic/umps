umpsdatadir = $(datadir)/umps2

umps_rom_types = coreboot tapeboot exec

dist_umpsdata_DATA = \
	coreboot.S	\
	tapeboot.S	\
	exec.S

dist_noinst_DATA = \
	coreboot.eb.rom.umps	\
	coreboot.el.rom.umps	\
	tapeboot.eb.rom.umps	\
	tapeboot.el.rom.umps	\
	exec.eb.rom.umps	\
	exec.el.rom.umps

if MAINTAINER_MODE
CLEANFILES = $(dist_noinst_DATA)
clean-local:
	-for t in $(umps_rom_types) ; do rm -f $$t.eb $$t.el; done

BIOS_CFLAGS = -mips1 -mabi=32 -mno-abicalls -fno-pic -ffreestanding -mfp32
BIOS_CPPFLAGS = -I$(top_srcdir)/src/include

SUFFIXES = .S .eb .el .eb.rom.umps .el.rom.umps

.S.eb:
	$(XCGCC) -c -EB $(BIOS_CPPFLAGS) $(BIOS_CFLAGS) -o $@ $<

.S.el:
	$(XCGCC) -c -EL $(BIOS_CPPFLAGS) $(BIOS_CFLAGS) -o $@ $<

.eb.eb.rom.umps:
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<

.el.el.rom.umps:
	$(top_builddir)/src/umps/umps2-elf2umps -v -b $<
endif

if WORDS_BIGENDIAN
endian = eb
else
endian = el
endif

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(umpsdatadir)
	@for t in $(umps_rom_types); do \
		rom=$$t.$(endian).rom.umps; \
		if test -f $$rom; then \
			f=$$rom; \
		else \
			f=$(srcdir)/$$rom; \
		fi; \
		$(INSTALL_DATA) $$f $(DESTDIR)$(umpsdatadir)/$$t.rom.umps; \
	done

uninstall-local:
	@for t in $(umps_rom_types); do \
		rm -f $(DESTDIR)$(umpsdatadir)/$$t.rom.umps; \
	done
