cmake_minimum_required(VERSION 3.17)
project( filetransferdaemon )

# defaults
option(WITH_LIBTORRENT "Build with libtorrent support" NO)


set(FTD_PREFIX /opt/bubba)


# compile the individual components as External Projects
include(ExternalProject)

# all External Projects temporary files to folder `_tmp`
set(EXT_TEMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_tmp)
# all External Projects deployment files in folder `_deploy`
set(EXT_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deploy)

ExternalProject_Add(sigc++
  SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/static-libs/libsigc++-2.4.1
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --enable-static --disable-shared --disable-documentation --prefix=${EXT_TARGET_DIR}
  PREFIX            ${EXT_TEMP_DIR}
)

# sigc++ install paths are weird, tidy up for the remainder of our project
add_custom_target(post-sigc++
  COMMAND           ln -snf ${EXT_TARGET_DIR}/include/sigc++-2.0/sigc++ ${EXT_TARGET_DIR}/include/sigc++ && ln -snf ${EXT_TARGET_DIR}/lib/sigc++-2.0/include/sigc++config.h ${EXT_TARGET_DIR}/include/sigc++config.h
  DEPENDS           sigc++
)

ExternalProject_Add(libeutils
  SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/static-libs/libeutils-0.7.39
  PATCH_COMMAND     patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patches/libeutils.patch
  CMAKE_ARGS        -DBUILD_STATIC_LIBRARIES=ON -DBUILD_SHARED_LIBRARIES=OFF -DCMAKE_INSTALL_PREFIX=${EXT_TARGET_DIR} -DSIGC++_CFLAGS=-I${EXT_TARGET_DIR}/include
  PREFIX            ${EXT_TEMP_DIR}
  DEPENDS           post-sigc++
)

# tcl-expect does not supply a pkg-config so we need to locate the library manually
file(GLOB LIBEXPECT /usr/lib*/expect*/libexpect*.so)
string(REGEX REPLACE "/libexpect.*$" "" LIBEXPECTPATH ${LIBEXPECT})
string(REGEX MATCH "expect.*$" LIBEXPECT ${LIBEXPECTPATH})
message("Found lib${LIBEXPECT}.so at ${LIBEXPECTPATH}")


set(FTD_PATCHFILE ${CMAKE_CURRENT_SOURCE_DIR}/patches/bubba-ftd.patch)

if(NOT WITH_LIBTORRENT)
  set(FTD_PATCHFILE ${FTD_PATCHFILE} ${CMAKE_CURRENT_SOURCE_DIR}/patches/systemd.patch)
endif()

ExternalProject_Add(bubba-ftd
  SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/bubba-ftd
  PATCH_COMMAND     cat ${FTD_PATCHFILE} | patch -p1
  BUILD_IN_SOURCE   OFF
  CONFIGURE_COMMAND mkdir -p ${EXT_TARGET_DIR}${FTD_PREFIX}/sbin
  BUILD_COMMAND     make DEBV=0 INCLUDEDIR=${EXT_TARGET_DIR}/include LIBDIR=${EXT_TARGET_DIR}/lib EXPECTDIR=${LIBEXPECTPATH} LIBEXPECT=${LIBEXPECT} -C <SOURCE_DIR>
  INSTALL_COMMAND   make DESTDIR=${EXT_TARGET_DIR} -C  <SOURCE_DIR> install
  PREFIX ${EXT_TEMP_DIR}
  DEPENDS libeutils
)



install(DIRECTORY ${EXT_TARGET_DIR}/opt ${EXT_TARGET_DIR}/usr ${EXT_TARGET_DIR}/var ${EXT_TARGET_DIR}/etc
        DESTINATION /
        PATTERN "*bin/*"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ
       )

