prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: libeutils
Description: Excito library routines
Version: @VERSION@
Requires.private: sigc++-2.0, glib-2.0
Libs: -L${libdir} -leutils -lpopt -pthread
Cflags: -I${includedir} -I${includedir}/tcl@TCL_VERSION@

