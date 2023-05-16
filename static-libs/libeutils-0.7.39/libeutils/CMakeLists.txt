
set( eutils_cpp_src
	AsyncWorker.cpp
	Condition.cpp
	DeferredWork.cpp
	ECGI.cpp
	EConfig.cpp
	EExcept.cpp
	Expect.cpp
	FileUtils.cpp
	FsTab.cpp
	Mutex.cpp
	NetClient.cpp
	NetDaemon.cpp
	PHPSession.cpp
	POpt.cpp
	Regex.cpp
	Serial.cpp
	Services.cpp
	SimpleCfg.cpp
	Socket.cpp
	StringTools.cpp
	Thread.cpp
	Url.cpp
	UserGroups.cpp
	Process.cpp
	SysvIPC.cpp
	SysvMsgQueue.cpp
	SysvShm.cpp
	)

set( eutils_hpp_src
	AsyncWorker.h
	Condition.h
	DeferredWork.h
	ECGI.h
	EConfig.h
	EExcept.h
	Expect.h
	FileUtils.h
	FsTab.h
	Mutex.h
	NetClient.h
	NetDaemon.h
	PHPSession.h
	POpt.h
	Regex.h
	Serial.h
	Services.h
	SimpleCfg.h
	Socket.h
	StringTools.h
	Thread.h
	Url.h
	UserGroups.h
	Process.h
	SysvIPC.h
	SysvMsgQueue.h
	SysvShm.h
	)

set(eutils_json_cpp_src
	json/json_reader.cpp
	json/json_value.cpp
	json/json_writer.cpp
	)

set(eutils_json_hpp_src
	json/json_batchallocator.h
	json/json_internalarray.inl
	json/json_internalmap.inl
	json/json_valueiterator.inl
	json/include/json/autolink.h
	json/include/json/config.h
	json/include/json/forwards.h
	json/include/json/json.h
	json/include/json/reader.h
	json/include/json/value.h
	json/include/json/writer.h
	)

include_directories( . ${libeutils_BINARY_DIR} json/include )
add_definitions( ${SIGC++_CFLAGS} ${GLIB_CFLAGS} -I${TCL_INCLUDE_PATH} )

if( BUILD_SHARED_LIBRARIES )
add_library(eutils_shared SHARED ${eutils_json_cpp_src} ${eutils_cpp_src})
target_link_libraries(eutils_shared pthread expect popt ${SIGC++_LDFLAGS} ${GLIB_LDFLAGS} )
set_target_properties(eutils_shared PROPERTIES
	VERSION ${libeutils_VERSION_FULL}
	SOVERSION ${libeutils_VERSION_MAJOR}
	OUTPUT_NAME eutils
	)

install(TARGETS eutils_shared DESTINATION lib)
endif()

if( BUILD_STATIC_LIBRARIES )
add_library(eutils_static STATIC ${eutils_json_cpp_src} ${eutils_cpp_src})
set_target_properties(eutils_static PROPERTIES
	VERSION ${libeutils_VERSION_FULL}
	SOVERSION ${libeutils_VERSION_MAJOR}
	OUTPUT_NAME eutils
	)
install(TARGETS eutils_static DESTINATION lib)
endif()




install(FILES ${eutils_hpp_src} DESTINATION ${libeutils_INCLUDE_DIR})
install(FILES ${eutils_json_hpp_src} DESTINATION ${libeutils_INCLUDE_DIR}/json)


