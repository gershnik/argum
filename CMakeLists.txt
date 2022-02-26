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
        inc/argum/argum.h
        inc/argum/common.h
        inc/argum/char-constants.h
        inc/argum/messages.h
        inc/argum/formatting.h
        inc/argum/partitioner.h
        inc/argum/flat-map.h
        inc/argum/data.h
        inc/argum/command-line.h
        inc/argum/tokenizer.h
        inc/argum/adaptive-parser.h
        inc/argum/validators.h

)

if (ARGUM_TEST)

    add_executable(test)

    set_property(TARGET test PROPERTY CXX_STANDARD 20)
    set_property(TARGET test PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET test PROPERTY CXX_VISIBILITY_PRESET hidden)
    set_property(TARGET test PROPERTY VISIBILITY_INLINES_HIDDEN ON)
    set_property(TARGET test PROPERTY POSITION_INDEPENDENT_CODE ON)


    target_link_libraries(test
        PRIVATE
            argum

            $<$<PLATFORM_ID:Android>:log>
    )

    target_compile_options(test 
        PRIVATE
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wall -Wextra -Wpedantic 
                -Wno-gnu-zero-variadic-macro-arguments #Clang bug - this is not an issue in C++20
                # -Weverything 
                # -Wno-c++98-compat 
                # -Wno-c++98-compat-pedantic 
                # -Wno-old-style-cast 
                # -Wno-ctad-maybe-unsupported
                # -Wno-return-std-move-in-c++11
                # -Wno-extra-semi-stmt
                # -Wno-shadow-uncaptured-local
                # -Wno-padded 
                # -Wno-weak-vtables
            > 
            
            $<$<CXX_COMPILER_ID:AppleClang>:-fprofile-instr-generate -fcoverage-mapping>

            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic
                -Wno-unknown-pragmas  #the whole point of pragmas it to be potentially unknown!
            >
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /Zc:preprocessor /W4 /WX>
    )

    target_link_options(test
        PRIVATE
            $<$<CXX_COMPILER_ID:AppleClang>:-fprofile-instr-generate -fcoverage-mapping>
    )

    target_sources(test 
        PRIVATE
            test/catch.hpp
            test/test.cpp
            test/test-formatting.cpp
            test/test-validators.cpp
            test/test-partitioner.cpp
            test/test-command-line.cpp
            test/test-tokenizer.cpp
            test/test-adaptive.cpp

            test/response.txt
            test/response1.txt
    )

    set(TEST_COMMAND "")
    set(TEST_DEPS "")

    if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
        set(ANDROID_TEST_DIR /data/local/tmp/sys_string_test)
        set(ANDROID_SDK_DIR ${CMAKE_ANDROID_NDK}/../..)
        set(ADB ${ANDROID_SDK_DIR}/platform-tools/adb)

        if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            set(ANDROID_LD_LIBRARY_PATH /apex/com.android.art/lib:/apex/com.android.runtime/lib)
        else()
            set(ANDROID_LD_LIBRARY_PATH /apex/com.android.art/lib64:/apex/com.android.runtime/lib64)
        endif()

        list(APPEND TEST_COMMAND COMMAND ${ADB} shell mkdir -p ${ANDROID_TEST_DIR}/data)
        list(APPEND TEST_COMMAND COMMAND ${ADB} push test ${ANDROID_TEST_DIR})
        list(APPEND TEST_COMMAND COMMAND ${ADB} push ${CMAKE_CURRENT_LIST_DIR}/test/response.txt ${ANDROID_TEST_DIR}/data)
        list(APPEND TEST_COMMAND COMMAND ${ADB} push ${CMAKE_CURRENT_LIST_DIR}/test/response1.txt ${ANDROID_TEST_DIR}/data)
        list(APPEND TEST_COMMAND COMMAND ${ADB} shell \"cd ${ANDROID_TEST_DIR} && LD_LIBRARY_PATH=${ANDROID_LD_LIBRARY_PATH} ./test\")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        list(APPEND TEST_COMMAND COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=$<TARGET_FILE_DIR:test>/test.profraw $<TARGET_FILE:test>)
        list(APPEND TEST_COMMAND COMMAND xcrun llvm-profdata merge -sparse $<TARGET_FILE_DIR:test>/test.profraw -o $<TARGET_FILE_DIR:test>/test.profdata)
        list(APPEND TEST_COMMAND COMMAND xcrun llvm-cov show -format=html 
                   -Xdemangler=c++filt -Xdemangler -n
                   -show-regions=1
                   -show-instantiations=0
                   -output-dir=${CMAKE_CURRENT_BINARY_DIR}/coverage 
                   -instr-profile=$<TARGET_FILE_DIR:test>/test.profdata 
                   $<TARGET_FILE:test>)
    else()
        list(APPEND TEST_COMMAND COMMAND test)
    endif()

    list(APPEND TEST_DEPS test)

    add_custom_target(run-test 
        DEPENDS ${TEST_DEPS}
        ${TEST_COMMAND}
    )

endif()
