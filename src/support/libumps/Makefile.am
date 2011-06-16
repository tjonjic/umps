umpsincludedir = $(includedir)/umps2/umps
umpsinclude_HEADERS = libumps.h types.h

umpsdatadir = $(datadir)/umps2
dist_umpsdata_DATA = libumps.S

install-data-hook:
	cd $(DESTDIR)$(umpsincludedir); \
	rm -f libumps.e; \
	$(LN_S) libumps.h libumps.e

uninstall-hook:
	cd $(DESTDIR)$(umpsincludedir); rm -f libumps.e
