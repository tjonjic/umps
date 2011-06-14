umpslibdir = $(libdir)/umps2
umpsdatadir = $(datadir)/umps2

legacy_ldscripts_eb = elf32btsmip.h.umpsaout.x elf32btsmip.h.umpscore.x
legacy_ldscripts_el = elf32ltsmip.h.umpsaout.x elf32ltsmip.h.umpscore.x

EXTRA_DIST = \
	Makefile.legacy_dist		\
	crtso_pic.S			\
	crtso_pic.o.eb			\
	crtso_pic.o.el			\
	crti_pic.S			\
	crti_pic.o.eb			\
	crti_pic.o.el			\
	$(legacy_ldscripts_eb)		\
	$(legacy_ldscripts_el)

#libumps_pic.o.eb
#libumps_pic.o.el

if WORDS_BIGENDIAN
endian = eb
umpsdata_DATA = $(legacy_ldscripts_eb)
else
endian = el
umpsdata_DATA = $(legacy_ldscripts_el)
endif

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(umpslibdir)
	$(INSTALL_DATA) $(srcdir)/crtso_pic.o.$(endian) $(DESTDIR)$(umpslibdir)/crtso.o
	$(INSTALL_DATA) $(srcdir)/crti_pic.o.$(endian) $(DESTDIR)$(umpslibdir)/crti.o

#$(INSTALL_DATA) $(srcdir)/libumps_pic.o.$(endian) $(DESTDIR)$(umps_libdir)/libumps.o

uninstall-local:
	-rm -f $(DESTDIR)$(umpslibdir)/crtso.o
	-rm -f $(DESTDIR)$(umpslibdir)/crti.o
#$(DESTDIR)$(umps_libdir)/libumps.o
