#
# Packaging
# https://cmake.org/cmake/help/latest/module/CPack.html
#
set( CPACK_PACKAGE_NAME ${PROJECT_NAME} )
set( CPACK_PACKAGE_VENDOR "Google" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Protocol Buffers - Google's data interchange format" )
set( CPACK_PACKAGE_HOMEPAGE_URL "https://developers.google.com/protocol-buffers/" )
set( CPACK_PACKAGE_CONTACT      "https://developers.google.com/protocol-buffers/" )
set( CPACK_PACKAGE_VERSION_MAJOR ${protobuf_VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${protobuf_VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${protobuf_VERSION_PATCH} )
set( CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
set( CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME} )
get_filename_component( protobuf_root_dir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY )
set( CPACK_RESOURCE_FILE_LICENSE "${protobuf_root_dir}/LICENSE" )
set( CPACK_RESOURCE_FILE_README "${protobuf_root_dir}/README.md" )

# Components are:
# - devel
# - libprotobuf-lite
# - libprotobuf

set( CPACK_COMPONENT_libprotobuf_DISPLAY_NAME "Protobuf Library" )
set( CPACK_COMPONENT_libprotobuf_DESCRIPTION "The Protobuf binary library." )
set( CPACK_COMPONENT_libprotobuf_REQUIRED 1 )
set( CPACK_COMPONENT_libprotobuf-lite_DISPLAY_NAME "Protobuf-lite Library" )
set( CPACK_COMPONENT_libprotobuf-lite_DESCRIPTION "The Protobuf-lite binary library." )
set( CPACK_COMPONENT_libprotobuf-lite_REQUIRED 0 )
set( CPACK_COMPONENT_devel_DISPLAY_NAME "Protobuf Development Files" )
set( CPACK_COMPONENT_devel_DESCRIPTION "Development files for compiling against Protobuf." )
set( CPACK_COMPONENT_devel_REQUIRED 0 )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

    if ( "${CPACK_PACKAGE_ARCHITECTURE}" STREQUAL "" )
        # Note: the architecture should default to the local architecture, but it
        # in fact comes up empty.  We call `uname -m` to ask the kernel instead.
        EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE )
    endif()

    set( CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1 )
    set( CPACK_PACKAGE_RELEASE 1 )


    # RPM - https://cmake.org/cmake/help/latest/cpack_gen/rpm.html
    set( CPACK_RPM_PACKAGE_RELEASE ${CPACK_PACKAGE_RELEASE} )
    set( CPACK_RPM_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
    set( CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION_SUMMARY} )
    set( CPACK_RPM_PACKAGE_URL ${CPACK_PACKAGE_HOMEPAGE_URL} )
    set( CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause" )
    set( CPACK_RPM_COMPONENT_INSTALL 1 )
    set( CPACK_RPM_COMPRESSION_TYPE "xz" )
    set( CPACK_RPM_PACKAGE_AUTOPROV 1 )

    set( CPACK_RPM_libprotobuf_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
    set( CPACK_RPM_libprotobuf_PACKAGE_NAME "libprotobuf" )
    set( CPACK_RPM_libprotobuf_FILE_NAME
         "${CPACK_RPM_libprotobuf_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_libprotobuf_PACKAGE_ARCHITECTURE}.rpm" )
    set( CPACK_RPM_libprotobuf_PACKAGE_SUMMARY ${CPACK_COMPONENT_libprotobuf_DESCRIPTION} )

    set( CPACK_RPM_libprotobuf-lite_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
    set( CPACK_RPM_libprotobuf-lite_PACKAGE_NAME "libprotobuf-lite" )
    set( CPACK_RPM_libprotobuf-lite_FILE_NAME
         "${CPACK_RPM_libprotobuf-lite_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_libprotobuf-lite_PACKAGE_ARCHITECTURE}.rpm" )
    set( CPACK_RPM_libprotobuf-lite_PACKAGE_SUMMARY ${CPACK_COMPONENT_libprotobuf-lite_DESCRIPTION} )

    set( CPACK_RPM_devel_PACKAGE_REQUIRES "cmake >= ${CMAKE_MINIMUM_REQUIRED_VERSION},libprotobuf >= ${CPACK_PACKAGE_VERSION},libprotobuf-lite >= ${CPACK_PACKAGE_VERSION}" )
    set( CPACK_RPM_devel_PACKAGE_SUMMARY ${CPACK_COMPONENT_devel_DESCRIPTION} )
    set( CPACK_RPM_devel_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
    set( CPACK_RPM_devel_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-devel" )
    set( CPACK_RPM_devel_FILE_NAME
         "${CPACK_RPM_devel_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_devel_PACKAGE_ARCHITECTURE}.rpm" )


    # DEB - https://cmake.org/cmake/help/latest/cpack_gen/deb.html
    set( CPACK_DEBIAN_PACKAGE_RELEASE ${CPACK_PACKAGE_RELEASE} )
    set( CPACK_DEBIAN_PACKAGE_HOMEPAGE ${CPACK_PACKAGE_HOMEPAGE_URL} )
    set( CPACK_DEB_COMPONENT_INSTALL 1 )
    set( CPACK_DEBIAN_COMPRESSION_TYPE "xz")

    if ( ${CPACK_PACKAGE_ARCHITECTURE} STREQUAL "x86_64" )
        set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64" )  # DEB doesn't always use the kernel's arch name
    else()
        set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CPACK_PACKAGE_ARCHITECTURE} )
    endif()

    set( CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT" ) # Use default naming scheme

    set( CPACK_DEBIAN_LIBPROTOBUF_PACKAGE_NAME "libprotobuf" )
    set( CPACK_DEBIAN_LIBPROTOBUF_PACKAGE_SHLIBDEPS 1 )
    set( CPACK_DEBIAN_LIBPROTOBUF-LITE_PACKAGE_NAME "libprotobuf-lite" )
    set( CPACK_DEBIAN_LIBPROTOBUF-LITE_PACKAGE_SHLIBDEPS 1 )

    set( CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS ${CPACK_RPM_devel_PACKAGE_REQUIRES} )
    set( CPACK_DEBIAN_DEVEL_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-dev" )

elseif( ${CMAKE_HOST_WIN32} )
    set( CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON )
    set( CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME} )
    set( CPACK_NSIS_PACKAGE_NAME ${PROJECT_NAME} )
    set( CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL} )
endif()
