# set target name
set(TARGET_NAME ril)

include_directories(BEFORE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/librilutils
    ${CMAKE_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

SYSR_INCLUDE_DIR(qmi-framework)
SYSR_INCLUDE_DIR(qmi-framework-vendor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES ril.cpp ril_event.cpp)

add_library (${TARGET_NAME} SHARED ${SOURCE_FILES})
target_link_libraries(${TARGET_NAME} rilutils)
target_link_libraries(${TARGET_NAME} protobuf-nanopb pthread
    ${GLIB_LIBRARIES} parcel mockcutils qrtr)
set_target_properties(${TARGET_NAME} PROPERTIES
                      LINK_FLAGS "-Wl,-z,defs"
                      VERSION 1.0.0
                      SOVERSION 1)

install ( TARGETS ${TARGET_NAME}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
