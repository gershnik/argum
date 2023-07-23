
find_package (Python3 COMPONENTS Interpreter)

if(${Python3_Interpreter_FOUND})

    add_custom_command(
        COMMENT "Generating amalgamated header"
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/argum.h
        WORKING_DIRECTORY ${TOOLSDIR}
        COMMAND ${Python3_EXECUTABLE} amalgamate.py template.txt ${CMAKE_CURRENT_BINARY_DIR}/argum.h -d ${SRCDIR}/inc/argum 
        DEPENDS 
            ${PUBLIC_HEADERS} 
            ${TOOLSDIR}/amalgamate.py
            ${TOOLSDIR}/template.txt
            ${TOOLSDIR}/module-template.txt
    )

    add_custom_command(
        COMMENT "Generating amalgamated module"
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/argum-module.ixx
        WORKING_DIRECTORY ${TOOLSDIR}
        COMMAND ${Python3_EXECUTABLE} amalgamate.py module-template.txt ${CMAKE_CURRENT_BINARY_DIR}/argum-module.ixx -d ${SRCDIR}/inc/argum 
        DEPENDS 
            ${PUBLIC_HEADERS} 
            ${TOOLSDIR}/amalgamate.py
            ${TOOLSDIR}/template.txt
            ${TOOLSDIR}/module-template.txt
    )

    add_custom_target(amalgamate
        COMMENT "Amalgamating"
        DEPENDS 
            ${CMAKE_CURRENT_BINARY_DIR}/argum-module.ixx
            ${CMAKE_CURRENT_BINARY_DIR}/argum.h
    )
 
endif()

