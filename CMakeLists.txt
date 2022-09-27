cmake_minimum_required(VERSION 3.9)

project(IfaceInformation
    VERSION 1.0.0
    DESCRIPTION "Library for get information about network interfaces")

option(MAKE_SHARED "Build library shared" ON)

if(EXISTS ${CMAKE_SOURCE_DIR}/version.cmake)
    message("version.h.in exist no need download")
else()
    file(DOWNLOAD
        https://gitlab.com/-/snippets/2417468/raw/main/version.cmake
        ${CMAKE_SOURCE_DIR}/version.cmake)
endif()


include(${CMAKE_SOURCE_DIR}/version.cmake)
_configure_information_header()

SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)



if(MAKE_SHARED)
    message("Build static library")
    add_library(${PROJECT_NAME} SHARED
        ifaceinformation.cpp
    )
else()
    message("Build static library")

    add_library(${PROJECT_NAME} STATIC
        ifaceinformation.h
        ifaceinformation.cpp
    )
endif()

message("Install path: ${LIBRARY_OUTPUT_DIRECTORY}")

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER networkinformer.h
    )


install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${LIBRARY_OUTPUT_PATH}
    PUBLIC_HEADER DESTINATION ${LIBRARY_OUTPUT_PATH})
