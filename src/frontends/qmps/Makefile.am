include $(top_srcdir)/build-aux/autotroll.mk

SUBDIRS = data

bin_PROGRAMS = umps2

umps2_moc_sources = \
	add_breakpoint_dialog.moc.cc \
	address_line_edit.moc.cc \
	add_suspect_dialog.moc.cc \
	add_tracepoint_dialog.moc.cc \
	application.moc.cc \
	boolean_item_delegate.moc.cc \
	code_view.moc.cc \
	code_view_priv.moc.cc \
	cpu_status_map.moc.cc \
	create_machine_dialog.moc.cc \
	debug_session.moc.cc \
	device_tree_model.moc.cc \
	device_tree_view.moc.cc \
	flat_push_button.moc.cc \
	hex_view.moc.cc \
	hex_view_priv.moc.cc \
	machine_config_dialog.moc.cc \
	machine_config_dialog_priv.moc.cc \
	machine_config_view.moc.cc \
	monitor_window.moc.cc \
	monitor_window_priv.moc.cc \
	processor_list_model.moc.cc \
	processor_window.moc.cc \
	register_set_snapshot.moc.cc \
	register_set_widget.moc.cc \
	stop_mask_view.moc.cc \
	stoppoint_list_model.moc.cc \
	suspect_type_delegate.moc.cc \
	symbol_table_model.moc.cc \
	terminal_view.moc.cc \
	terminal_window.moc.cc \
	terminal_window_priv.moc.cc \
	tlb_model.moc.cc \
	trace_browser.moc.cc \
	trace_browser_priv.moc.cc \
	tree_view.moc.cc

umps2_SOURCES = \
	main.cc				\
	application.h			\
	application.cc			\
	debug_session.h			\
	debug_session.cc		\
	monitor_window.h		\
	monitor_window_priv.h		\
	monitor_window.cc		\
	error_hooks.cc			\
	create_machine_dialog.h		\
	create_machine_dialog.cc	\
	stop_mask_view.h		\
	stop_mask_view.cc		\
	processor_list_model.h		\
	processor_list_model.cc		\
	machine_config_dialog.h		\
	machine_config_dialog_priv.h	\
	machine_config_dialog.cc	\
	machine_config_view.h		\
	machine_config_view.cc		\
	trace_browser.h			\
	trace_browser_priv.h		\
	trace_browser.cc		\
	memory_view_delegate.h		\
	hex_view.h			\
	hex_view_priv.h			\
	hex_view.cc			\
	symbol_table_model.h		\
	symbol_table_model.cc		\
	processor_window.h		\
	processor_window.cc		\
	code_view.h			\
	code_view_priv.h		\
	code_view.cc			\
	register_set_snapshot.h		\
	register_set_snapshot.cc	\
	register_set_widget.h		\
	register_set_widget.cc		\
	tlb_model.h			\
	tlb_model.cc			\
	terminal_view.h			\
	terminal_view.cc		\
	terminal_window.h		\
	terminal_window_priv.h		\
	terminal_window.cc		\
	cpu_status_map.h		\
	cpu_status_map.cc		\
	stoppoint_list_model.h		\
	stoppoint_list_model.cc		\
	suspect_type_delegate.h		\
	suspect_type_delegate.cc	\
	device_tree_view.h		\
	device_tree_view.cc		\
	device_tree_model.h		\
	device_tree_model.cc		\
	add_breakpoint_dialog.h		\
	add_breakpoint_dialog.cc	\
	add_suspect_dialog.h		\
	add_suspect_dialog.cc		\
	add_tracepoint_dialog.h		\
	add_tracepoint_dialog.cc	\
	address_line_edit.h		\
	address_line_edit.cc		\
	mac_id_edit.h			\
	mac_id_edit.cc			\
	boolean_item_delegate.h		\
	boolean_item_delegate.cc	\
	register_item_delegate.h	\
	register_item_delegate.cc	\
	ui_utils.h			\
	ui_utils.cc			\
	tree_view.h			\
	tree_view.cc			\
	flat_push_button.h		\
	flat_push_button.cc

nodist_umps2_SOURCES = \
	qmps.qrc.cc			\
	$(umps2_moc_sources)

umps2_CXXFLAGS = $(QT_CXXFLAGS) $(AM_CXXFLAGS)

umps2_CPPFLAGS = \
	$(UMPSCPPFLAG) $(QT_CPPFLAGS) $(SIGCPP_CFLAGS) \
	-I$(top_srcdir)/src -I$(top_srcdir)/src/include	-I$(top_srcdir)/src/frontends

umps2_LDFLAGS = $(QT_LDFLAGS)

umps2_DEPENDENCIES = \
	$(top_builddir)/src/base/libbase.a	\
	$(top_builddir)/src/umps/libumps.a

umps2_LDADD = \
	$(DL_LIBS)				\
	$(top_builddir)/src/umps/libumps.a	\
	$(top_builddir)/src/base/libbase.a	\
	$(QT_LIBS)				\
	$(SIGCPP_LIBS)

BUILT_SOURCES = $(umps2_moc_sources) qmps.qrc.cc

EXTRA_DIST = qmps.qrc icons
