#  Copyright 2022 Eugene Gershnik
#
#  Use of this source code is governed by a BSD-style
#  license that can be found in the LICENSE file or at
#  https://github.com/gershnik/argum/blob/master/LICENSE
#
cmake_minimum_required(VERSION 3.25)

file(READ VERSION ARGUM_VERSION)
if (NOT ARGUM_VERSION)
    message(FATAL_ERROR "Cannot determine library version (VERSION file not found)")
endif()
string(STRIP ${ARGUM_VERSION} ARGUM_VERSION)

option(ARGUM_NO_TESTS "suppress test targets" OFF)

project(argum VERSION ${ARGUM_VERSION} LANGUAGES CXX)

add_library(argum INTERFACE)

set(SRCDIR ${CMAKE_CURRENT_LIST_DIR})
set(TOOLSDIR ${SRCDIR}/.tools)

target_include_directories(argum 
    INTERFACE
        $<BUILD_INTERFACE:${SRCDIR}/inc>
        $<INSTALL_INTERFACE:include>  # <prefix>/include
)

target_compile_definitions(argum
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

if ("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
    target_compile_definitions(argum
        INTERFACE
            $<$<CXX_COMPILER_ID:Clang>:_CRT_SECURE_NO_WARNINGS>
    )
endif()

set(PUBLIC_HEADERS 
    ${SRCDIR}/inc/argum/common.h
    ${SRCDIR}/inc/argum/char-constants.h
    ${SRCDIR}/inc/argum/messages.h
    ${SRCDIR}/inc/argum/color.h
    ${SRCDIR}/inc/argum/formatting.h
    ${SRCDIR}/inc/argum/partitioner.h
    ${SRCDIR}/inc/argum/flat-map.h
    ${SRCDIR}/inc/argum/simple-file.h
    ${SRCDIR}/inc/argum/data.h
    ${SRCDIR}/inc/argum/expected.h
    ${SRCDIR}/inc/argum/command-line.h
    ${SRCDIR}/inc/argum/tokenizer.h
    ${SRCDIR}/inc/argum/parser.h
    ${SRCDIR}/inc/argum/validators.h
    ${SRCDIR}/inc/argum/help-formatter.h
    ${SRCDIR}/inc/argum/type-parsers.h
    ${SRCDIR}/inc/argum/wcwidth.h
)

target_sources(argum
INTERFACE 
    FILE_SET HEADERS BASE_DIRS ${SRCDIR}/inc FILES
        ${PUBLIC_HEADERS}
PRIVATE 
    ${PUBLIC_HEADERS}
)

add_library(argum::argum ALIAS argum)

if (PROJECT_IS_TOP_LEVEL)

    include(cmake/install.cmake)
    include(cmake/amalgamate.cmake)

    if (NOT ARGUM_NO_TESTS)
        include(test/test.cmake)
        include(samples/samples.cmake)
    endif()

endif()
