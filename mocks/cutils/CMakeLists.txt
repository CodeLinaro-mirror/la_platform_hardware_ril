set( CMAKE_CONFIGURATION_TYPES
        "Debug"
        "Release"
        )
set( CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Available build configurations." FORCE )

include_directories(BEFORE
        ${CMAKE_SOURCE_DIR}/mocks )

set( TARGET_MOCK_LIBCUTILS mockcutils )

set( MOCK_CUTILS_SOURCES
        sockets2.c
        properties2.c
        ril_config.c
        )

if (NOT RIL_FOR_EXTERNAL_AP)
    add_definitions(-DSOCKET_PATH_PREFIX="/dev/socket/ril/")
endif (NOT RIL_FOR_EXTERNAL_AP)

# build library
add_library ( ${TARGET_MOCK_LIBCUTILS} SHARED
        ${MOCK_CUTILS_SOURCES}
        )

target_link_libraries(${TARGET_MOCK_LIBCUTILS}
    ${GLIB_LIBRARIES})

set_target_properties(${TARGET_MOCK_LIBCUTILS} PROPERTIES
        VERSION 1.0.0
        SOVERSION 1)

# install to target
install ( TARGETS ${TARGET_MOCK_LIBCUTILS}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )

set( MOCK_CUTILS_HEADERS
        sockets2.h
        properties2.h
        ril_config.h
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

install( FILES
        ${MOCK_CUTILS_HEADERS}
        DESTINATION include/cutils )
