cmake_minimum_required(VERSION 3.9)

project(iface_information
    VERSION 1.1.0
    DESCRIPTION "Library for get information about network interfaces")

message(STATUS "CMake version   : " ${CMAKE_SYSTEM_VERSION})
message(STATUS "Compiler        : " ${CMAKE_CXX_COMPILER})
message(STATUS "Operating System: " ${CMAKE_SYSTEM})
message(STATUS "Arch            : " ${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_CXX_STANDARD 17)

option(MAKE_SHARED "Build library shared" OFF)

include( GNUInstallDirs )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

########################################################
#                  Файл для версионирования           #
########################################################
if(NOT EXISTS ${CMAKE_MODULE_PATH}/version/version.cmake)
    file(DOWNLOAD
        https://gitlab.com/-/snippets/2417468/raw/main/version.cmake
        ${CMAKE_MODULE_PATH}/version/version.cmake)
endif()

include(${CMAKE_MODULE_PATH}/version/version.cmake)

_configure_information_header(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/include/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/include/version.h
    )


########################################################
#                 Выбор файлов для компиляции          #
########################################################
if(CMAKE_HOST_SYSTEM MATCHES "Linux")
    set(IFACELIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/ifaceinformation.cpp)
    set(IFACELIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/include/ifaceinformation.h)
else()
    message(FATAL "Unable to build for your system")
endif()



########################################################
#                   Тип сборки                         #
########################################################
if(MAKE_SHARED)
    message(STATUS "Interface library build shared")
    add_library(${PROJECT_NAME} SHARED ${IFACELIB_SOURCES} ${IFACELIB_HEADERS})
    set_target_properties ( ${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER ${IFACELIB_HEADERS}
        VERSION ${PROJECT_VERSION} )
else()

    message("Interface library build static")
    add_library(${PROJECT_NAME} STATIC
        ${IFACELIB_HEADERS}
        ${IFACELIB_SOURCES}
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/include>)
message(STATUS "Install path: ${CMAKE_INSTALL_LIBDIR}")
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_LIBDIR}/include
    )
