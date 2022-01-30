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

    target_sources(test 
        PRIVATE
            test/catch.hpp
            test/test.cpp
            test/test-tokenizer.cpp
            test/test-sequential.cpp
    )

endif()
