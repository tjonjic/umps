umps_libdir = $(libdir)/umps2

EXTRA_DIST = \
	Makefile.legacy_dist	\
	crtso_pic.S		\
	crtso_pic.o.eb		\
	crtso_pic.o.el

#libumps_pic.o.eb
#libumps_pic.o.el

if WORDS_BIGENDIAN
endian = eb
else
endian = el
endif

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(umps_libdir)
	$(INSTALL_DATA) $(srcdir)/crtso_pic.o.$(endian) $(DESTDIR)$(umps_libdir)/crtso.o
#$(INSTALL_DATA) $(srcdir)/libumps_pic.o.$(endian) $(DESTDIR)$(umps_libdir)/libumps.o

uninstall-local:
	-rm -f $(DESTDIR)$(umps_libdir)/crtso.o
#$(DESTDIR)$(umps_libdir)/libumps.o
