# The one and only required CMake script to build protoc and/or library

# Basic configuration
IF(NOT WIN32)
	FIND_PACKAGE(Threads REQUIRED)
	SET(HAVE_PTHREAD CMAKE_HAVE_PTHREAD_H)
ENDIF()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

INIT_BIICODE_BLOCK()
ADD_BIICODE_TARGETS()

if(TARGET google_protobuf_compiler_main) # rename 2 protoc
	SET_TARGET_PROPERTIES(google_protobuf_compiler_main PROPERTIES OUTPUT_NAME protoc)
endif()
IF(BII_LIB_SRC)
	target_include_directories(${BII_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
ELSE()
	target_include_directories(${BII_LIB_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
ENDIF()