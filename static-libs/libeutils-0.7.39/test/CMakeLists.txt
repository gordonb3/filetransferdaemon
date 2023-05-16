if( TUT_FOUND )
    SET(eutilstest_SRCS
        test_main.cpp
        ProcessTest.cpp
        CGITest.cpp
        )
    include_directories( . ${libeutils_BINARY_DIR} )
    link_directories( ${libeutils_BINARY_DIR}/libeutils )

    add_executable(eutilstest ${eutilstest_SRCS})

    add_definitions( ${TUT_CFLAGS} ${SIGC++_CFLAGS} ${GLIB_CFLAGS} )

    target_link_libraries(eutilstest eutils dl popt expect ${TUT_LDFLAGS} ${SIGC++_LDFLAGS} ${GLIB_LDFLAGS} )

    enable_testing()
    add_test( "EUtils" eutilstest )
endif( TUT_FOUND )
