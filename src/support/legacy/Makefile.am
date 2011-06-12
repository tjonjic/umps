umps_libdir = $(libdir)/umps2

EXTRA_DIST = \
	Makefile.legacy_dist	\
	crtso_pic.S		\
	crtso_pic.eb.o		\
	crtso_pic.el.o		\
	libumps_pic.eb.o	\
	libumps_pic.el.o

if WORDS_BIGENDIAN
endian = eb
else
endian = el
endif

install-data-local:
	$(INSTALL_DATA) $(srcdir)/crtso_pic.$(endian).o $(DESTDIR)$(umps_libdir)/crtso.o
	$(INSTALL_DATA) $(srcdir)/libumps_pic.$(endian).o $(DESTDIR)$(umps_libdir)/libumps.o

uninstall-local:
	-rm -f $(DESTDIR)$(umps_libdir)/crtso.o \
	       $(DESTDIR)$(umps_libdir)/libumps.o
