
find_package (Python3 COMPONENTS Interpreter)

if(${Python3_Interpreter_FOUND})

    add_custom_command(
        COMMENT "Generating amalgamated header"
        OUTPUT ${SRCDIR}/single-file/argum.h
        WORKING_DIRECTORY ${TOOLSDIR}
        COMMAND ${Python3_EXECUTABLE} amalgamate.py -d ${SRCDIR}/inc/argum template.txt ${SRCDIR}/single-file/argum.h 
        DEPENDS 
            ${PUBLIC_HEADERS} 
            ${TOOLSDIR}/amalgamate.py
            ${TOOLSDIR}/template.txt
            ${TOOLSDIR}/module-template.txt
    )

    add_custom_command(
        COMMENT "Generating amalgamated module"
        OUTPUT ${SRCDIR}/single-file/argum-module.ixx
        WORKING_DIRECTORY ${TOOLSDIR}
        COMMAND ${Python3_EXECUTABLE} amalgamate.py -d ${SRCDIR}/inc/argum module-template.txt ${SRCDIR}/single-file/argum-module.ixx
        DEPENDS 
            ${PUBLIC_HEADERS} 
            ${TOOLSDIR}/amalgamate.py
            ${TOOLSDIR}/template.txt
            ${TOOLSDIR}/module-template.txt
    )

    add_custom_target(amalgamate
        COMMENT "Amalgamating"
        DEPENDS 
            ${SRCDIR}/single-file/argum-module.ixx
            ${SRCDIR}/single-file/argum.h
    )
 
endif()

