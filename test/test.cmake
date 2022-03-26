#  Copyright 2022 Eugene Gershnik
#
#  Use of this source code is governed by a BSD-style
#  license that can be found in the LICENSE file or at
#  https://github.com/gershnik/argum/blob/master/LICENSE
#

find_package (Python3 COMPONENTS Interpreter)


function(configure_test name)

    set_property(TARGET ${name} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET hidden)
    set_property(TARGET ${name} PROPERTY VISIBILITY_INLINES_HIDDEN ON)
    set_property(TARGET ${name} PROPERTY POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${name}
        PRIVATE
            argum

            $<$<PLATFORM_ID:Android>:log>
    )

    target_compile_options(${name} 
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
            
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic
                -Wno-unknown-pragmas  #the whole point of pragmas it to be potentially unknown!
            >
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /Zc:preprocessor /W4 /WX>
    )

    target_sources(${name} 
        PRIVATE
            test/catch.hpp
            test/test.cpp
            test/test-common.h
            test/test-formatting.cpp
            test/test-validators.cpp
            test/test-partitioner.cpp
            test/test-command-line.cpp
            test/test-tokenizer.cpp
            test/parser-common.h
            test/test-parser-options.cpp
            test/test-parser-positionals.cpp
            test/test-parser-options-positionals.cpp
            test/test-parser-help.cpp
            test/test-parser-adaptive.cpp
            test/test-parser-validation.cpp
            test/test-type-parsers.cpp

            test/response.txt
            test/response1.txt
    )
    
endfunction(configure_test name)



add_executable(test)
configure_test(test)

target_compile_options(test
    PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang>:-fprofile-instr-generate -fcoverage-mapping>
)

target_link_options(test
    PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang>:-fprofile-instr-generate -fcoverage-mapping>
)

add_executable(test_expected)
configure_test(test_expected)

target_compile_definitions(test_expected 
    PRIVATE 
        ARGUM_USE_EXPECTED 
)

add_executable(test_nothrow)
configure_test(test_nothrow)

target_compile_definitions(test_nothrow 
    PRIVATE 
        ARGUM_NO_THROW 
)

target_compile_options(test_nothrow
    PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang>:-fno-exceptions>
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
    list(APPEND TEST_COMMAND COMMAND ${ADB} push test_expected ${ANDROID_TEST_DIR})
    list(APPEND TEST_COMMAND COMMAND ${ADB} push test_nothrow ${ANDROID_TEST_DIR})
    list(APPEND TEST_COMMAND COMMAND ${ADB} push ${CMAKE_CURRENT_LIST_DIR}/response.txt ${ANDROID_TEST_DIR}/data)
    list(APPEND TEST_COMMAND COMMAND ${ADB} push ${CMAKE_CURRENT_LIST_DIR}/response1.txt ${ANDROID_TEST_DIR}/data)
    list(APPEND TEST_COMMAND COMMAND ${ADB} shell \"cd ${ANDROID_TEST_DIR} && LD_LIBRARY_PATH=${ANDROID_LD_LIBRARY_PATH} ./test\")
    list(APPEND TEST_COMMAND COMMAND ${ADB} shell \"cd ${ANDROID_TEST_DIR} && LD_LIBRARY_PATH=${ANDROID_LD_LIBRARY_PATH} ./test_expected\")
    list(APPEND TEST_COMMAND COMMAND ${ADB} shell \"cd ${ANDROID_TEST_DIR} && LD_LIBRARY_PATH=${ANDROID_LD_LIBRARY_PATH} ./test_nothrow\")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    list(APPEND TEST_COMMAND COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=$<TARGET_FILE_DIR:test>/test.profraw $<TARGET_FILE:test>)
    list(APPEND TEST_COMMAND COMMAND xcrun llvm-profdata merge -sparse $<TARGET_FILE_DIR:test>/test.profraw -o $<TARGET_FILE_DIR:test>/test.profdata)
    list(APPEND TEST_COMMAND COMMAND xcrun llvm-cov show -format=html 
                -Xdemangler=c++filt -Xdemangler -n
                -show-regions=1
                -show-instantiations=0
                #-show-branches=count
                #-show-instantiation-summary=1
                -ignore-filename-regex=test/.\\*
                -output-dir=${CMAKE_CURRENT_BINARY_DIR}/coverage 
                -instr-profile=$<TARGET_FILE_DIR:test>/test.profdata 
                $<TARGET_FILE:test>)
    list(APPEND TEST_COMMAND COMMAND test_expected)
    list(APPEND TEST_COMMAND COMMAND test_nothrow)
else()
    list(APPEND TEST_COMMAND COMMAND test)
    list(APPEND TEST_COMMAND COMMAND test_expected)
    list(APPEND TEST_COMMAND COMMAND test_nothrow)
endif()

list(APPEND TEST_DEPS test test_expected test_nothrow)

add_custom_target(run-test 
    DEPENDS ${TEST_DEPS}
    ${TEST_COMMAND}
)

if(${Python3_Interpreter_FOUND})

    add_custom_target(amalgamate
        COMMENT "Amalgamating"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../tools
        COMMAND ${Python3_EXECUTABLE} amalgamate.py template.txt ../out/argum.h -d ../inc/argum 
        COMMAND ${Python3_EXECUTABLE} amalgamate.py module-template.txt ../out/argum-module.ixx -d ../inc/argum 
        DEPENDS 
            ${UNICODE_DATA} 
            ${UNICODE_SCRIPTS}
    )

    add_dependencies(test amalgamate)
 
endif()