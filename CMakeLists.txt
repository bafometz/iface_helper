cmake_minimum_required(VERSION 3.9)

project(iface_information
    VERSION 1.0.0
    DESCRIPTION "Library for get information about network interfaces")

message(STATUS "CMake version   : " ${CMAKE_SYSTEM_VERSION})
message(STATUS "Compiler        : " ${CMAKE_CXX_COMPILER})
message(STATUS "Operating System: " ${CMAKE_SYSTEM})
message(STATUS "Arch            : " ${CMAKE_SYSTEM_PROCESSOR})

set(CMAKE_CXX_STANDARD 17)

option(MAKE_SHARED "Build library shared" ON)

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
_configure_information_header(${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaceInformation/version.h)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)


########################################################
#                 Выбор файлов для компиляции          #
########################################################
if(CMAKE_HOST_SYSTEM MATCHES "Linux")
    set(IFACELIB_SOURCES src/interfaceInformation/ifaceinformation.cpp)
    set(IFACELIB_HEADERS src/interfaceInformation/ifaceinformation.h)
else()
    message(FATAL "Unable to build for your system")
endif()


########################################################
#                   Тип сборки                         #
########################################################
if(MAKE_SHARED)
    message("SHARED BUILD")
    add_library(${PROJECT_NAME} SHARED ${IFACELIB_SOURCES})

    set_target_properties ( ${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER ${IFACELIB_HEADERS}
        RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}
        VERSION ${PROJECT_VERSION} )
else()

    message("STATIC BUILD")
    add_library(${PROJECT_NAME} STATIC
        ${IFACELIB_HEADERS}
        ${IFACELIB_SOURCES}
    )
endif()

message("Install path: ${LIBRARY_OUTPUT_PATH}")

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${IFACELIB_HEADERS}
    )

include( GNUInstallDirs )
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${LIBRARY_OUTPUT_PATH}
    PUBLIC_HEADER DESTINATION ${LIBRARY_OUTPUT_PATH}/include
    )
