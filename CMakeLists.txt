cmake_minimum_required(VERSION 2.8.9)

project (ril)

include(GNUInstallDirs)
include(FindPkgConfig)

pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS} -include glib.h)
add_definitions(-Dstrlcpy=g_strlcpy)
add_definitions(-Dstrlcat=g_strlcat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/qmi-framework-vendor -Wnon-virtual-dtor")

# Supported configurations
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
  )
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Available build configurations." FORCE)


add_definitions(-DRIL_FOR_MDM_LE)

set(SYSTEMD_UNITDIR "${CMAKE_INSTALL_PREFIX}/../lib/systemd/system" CACHE PATH
        "Set directory to install systemd unit files")

if (RIL_FOR_EXTERNAL_AP)
    add_definitions(-DRIL_FOR_EXTERNAL_AP)
    # Feature flag used by some data header files
    add_definitions(-DFEATURE_EXTERNAL_AP)
    if (NOT MACHINE_HAS_CV2X_ONLY)
        install(FILES scripts/rild.service
                DESTINATION ${SYSTEMD_UNITDIR})
        install(FILES scripts/rild2.service
                DESTINATION ${SYSTEMD_UNITDIR})
        install(FILES scripts/rild_launcher.sh
                PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
                DESTINATION /etc/initscripts)
    endif (NOT MACHINE_HAS_CV2X_ONLY)
endif (RIL_FOR_EXTERNAL_AP)

include_directories(BEFORE include)

if (NOT MACHINE_HAS_CV2X_ONLY)
    add_subdirectory( rild )
endif (NOT MACHINE_HAS_CV2X_ONLY)
add_subdirectory( librilutils )
add_subdirectory( libril )
add_subdirectory( mocks )

# <--------------------------------------------->
# adding header file install
# <--------------------------------------------->
install( DIRECTORY include/libril DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( DIRECTORY include/telephony DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )

# <--------------------------------------------->
# adding RPM generation
# <--------------------------------------------->
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION "1.0")
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Automotive BU")
set(CPACK_PACKAGE_VENDOR "QTI Inc.")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES "/raw/" "/\\\\.git/")
include(CPack)
