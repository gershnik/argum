cmake_minimum_required(VERSION "3.20")

project(modern-arg-parser)

add_library(modern-arg-parser INTERFACE)

target_include_directories(modern-arg-parser 
    INTERFACE
        inc
)

target_sources(modern-arg-parser
    PRIVATE
        inc/modern-arg-parser/modern-arg-parser.h
        inc/modern-arg-parser/char-constants.h
        inc/modern-arg-parser/messages.h
        inc/modern-arg-parser/formatting.h
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
    )

    target_compile_options(test 
        PRIVATE
            $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra -Wpedantic -Weverything 
                -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-padded -Wno-unused-function -Wno-ctad-maybe-unsupported>
    )

    target_sources(test 
        PRIVATE
            test/catch.hpp
            test/test.cpp
            test/test-tokenizer.cpp
            test/test-sequential.cpp
    )

endif()
