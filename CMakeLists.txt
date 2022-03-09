#  Copyright 2022 Eugene Gershnik
#
#  Use of this source code is governed by a BSD-style
#  license that can be found in the LICENSE file or at
#  https://github.com/gershnik/argum/blob/master/LICENSE
#
cmake_minimum_required(VERSION "3.20")

project(argum)

add_library(argum INTERFACE)

target_include_directories(argum 
    INTERFACE
        inc
)

target_compile_definitions(argum
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_sources(argum
    PRIVATE
        inc/argum/common.h
        inc/argum/char-constants.h
        inc/argum/messages.h
        inc/argum/formatting.h
        inc/argum/partitioner.h
        inc/argum/flat-map.h
        inc/argum/simple-file.h
        inc/argum/data.h
        inc/argum/command-line.h
        inc/argum/tokenizer.h
        inc/argum/parser.h
        inc/argum/validators.h
        inc/argum/help-formatter.h
        inc/argum/type-parsers.h

)

if (ARGUM_TEST)

    include(test/test.cmake)

endif()
