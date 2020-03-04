cmake_minimum_required(VERSION 2.8.9)

project (ril)

include(GNUInstallDirs)
include(FindPkgConfig)

set(CMAKE_SHARED_LINKER_FLAGS "${LDFLAGS} -z noexecstack -z relro -z now")
set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS} -z noexecstack -z relro -z now")

pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS} -include glib.h)
add_definitions(-Dstrlcpy=g_strlcpy)
add_definitions(-Dstrlcat=g_strlcat)

# Supported configurations
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
  )
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Available build configurations." FORCE)

include_directories(BEFORE include)

add_subdirectory( rild )
add_subdirectory( librilutils )
add_subdirectory( libril )

# <--------------------------------------------->
# adding header file install
# <--------------------------------------------->
install( DIRECTORY include/libril DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( DIRECTORY include/telephony DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )

