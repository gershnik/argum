cmake_minimum_required(VERSION "3.20")

project(modern-arg-parser)

add_library(modern-arg-parser INTERFACE)

target_include_directories(modern-arg-parser 
    INTERFACE
        inc
)

target_compile_definitions(modern-arg-parser
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

target_sources(modern-arg-parser
    PRIVATE
        inc/modern-arg-parser/modern-arg-parser.h
        inc/modern-arg-parser/common.h
        inc/modern-arg-parser/char-constants.h
        inc/modern-arg-parser/messages.h
        inc/modern-arg-parser/formatting.h
        inc/modern-arg-parser/partitioner.h
        inc/modern-arg-parser/flat-map.h
        inc/modern-arg-parser/data.h
        inc/modern-arg-parser/tokenizer.h
        inc/modern-arg-parser/adaptive-parser.h
        inc/modern-arg-parser/validators.h

)

if (MODERN_ARG_PARSER_TEST)

    add_executable(test)

    set_property(TARGET test PROPERTY CXX_STANDARD 20)
    set_property(TARGET test PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET test PROPERTY CXX_VISIBILITY_PRESET hidden)
    set_property(TARGET test PROPERTY VISIBILITY_INLINES_HIDDEN ON)
    set_property(TARGET test PROPERTY POSITION_INDEPENDENT_CODE ON)


    target_link_libraries(test
        PRIVATE
            modern-arg-parser

            $<$<PLATFORM_ID:Android>:log>
    )

    target_compile_options(test 
        PRIVATE
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wall -Wextra -Wpedantic -Weverything 
                -Wno-c++98-compat 
                -Wno-c++98-compat-pedantic 
                -Wno-old-style-cast 
                -Wno-ctad-maybe-unsupported
                -Wno-return-std-move-in-c++11
                -Wno-extra-semi-stmt
                -Wno-shadow-uncaptured-local
                -Wno-padded 
                -Wno-weak-vtables> 
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W4 /WX>
    )

    target_sources(test 
        PRIVATE
            test/catch.hpp
            test/test.cpp
            test/test-formatting.cpp
            test/test-validators.cpp
            test/test-partitioner.cpp
            test/test-tokenizer.cpp
            test/test-adaptive.cpp
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

        list(APPEND TEST_COMMAND COMMAND ${ADB} shell mkdir -p ${ANDROID_TEST_DIR})
        list(APPEND TEST_COMMAND COMMAND ${ADB} push test ${ANDROID_TEST_DIR})
        list(APPEND TEST_COMMAND COMMAND ${ADB} shell LD_LIBRARY_PATH=${ANDROID_LD_LIBRARY_PATH} ${ANDROID_TEST_DIR}/test)
    else()
        list(APPEND TEST_COMMAND COMMAND test)
    endif()

    list(APPEND TEST_DEPS test)

    add_custom_target(run-test 
        DEPENDS ${TEST_DEPS}
        ${TEST_COMMAND}
    )

endif()
