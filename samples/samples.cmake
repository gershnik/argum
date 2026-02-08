
add_executable(sample-demo EXCLUDE_FROM_ALL samples/demo.cpp)
list(APPEND samples sample-demo)

add_executable(sample-basics EXCLUDE_FROM_ALL samples/basics.cpp)
list(APPEND samples sample-basics)

foreach(sample IN ITEMS ${samples})
    
    if (NOT DEFINED CMAKE_CXX_STANDARD)
        set_property(TARGET ${sample} PROPERTY CXX_STANDARD 20)
        set_property(TARGET ${sample} PROPERTY CXX_STANDARD_REQUIRED ON)
    endif()

    target_compile_definitions(${sample}
        PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    )

    if ("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
        target_compile_definitions(${sample}
            PRIVATE
                $<$<CXX_COMPILER_ID:Clang>:_CRT_SECURE_NO_WARNINGS>
        )
    endif()

    if (TARGET amalgamate)
        add_dependencies(${sample} amalgamate)
    endif()

endforeach()


add_custom_target(samples
    DEPENDS ${samples}
)
