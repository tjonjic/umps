AM_CPPFLAGS = $(UMPSCPPFLAG) -I$(top_srcdir)/src -I$(top_srcdir)/src/include

noinst_LIBRARIES = libumps.a

libumps_a_SOURCES = \
	blockdev.h		\
	blockdev.cc		\
	blockdev_params.h	\
	const.h			\
	device.h		\
	device.cc		\
	disassemble.h		\
	disassemble.cc		\
	error.h			\
	event.h			\
	event.cc		\
	machine_config.h	\
	machine_config.cc	\
	machine.h		\
	machine.cc		\
	memspace.h		\
	memspace.cc		\
	mp_controller.h		\
	mp_controller.cc	\
	mpic.h			\
	mpic.cc			\
	processor.h		\
	processor.cc		\
	processor_defs.h	\
	stoppoint.h		\
	stoppoint.cc		\
	symbol_table.h		\
	symbol_table.cc		\
	systembus.h		\
	systembus.cc		\
	time_stamp.h		\
	time_stamp.cc		\
	types.h			\
	utility.h		\
	utility.cc		\
	vde_network.h		\
	vde_network.cc		\
	libvdeplug_dyn.h

libumps_a_CPPFLAGS = \
	$(AM_CPPFLAGS) $(SIGCPP_CFLAGS)	\
	-DPACKAGE_DATA_DIR="\"$(datadir)/umps2\""

bin_PROGRAMS = umps2-elf2umps umps2-mkdev umps2-objdump

umps2_elf2umps_SOURCES = \
	elf2umps.cc

umps2_elf2umps_LDADD = $(ELF_LIBS)

umps2_mkdev_SOURCES = \
	mkdev.cc

umps2_objdump_SOURCES = \
	disassemble.cc		\
	objdump.cc
