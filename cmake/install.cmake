#  Copyright 2022 Eugene Gershnik
#
#  Use of this source code is governed by a BSD-style
#  license that can be found in the LICENSE file or at
#  https://github.com/gershnik/argum/blob/master/LICENSE
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS argum EXPORT argum FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT argum NAMESPACE argum:: FILE argum-exports.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/argum)

configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/argum-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/argum-config.cmake
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/argum
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/argum-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/argum-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/argum-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/argum
)

file(RELATIVE_PATH FROM_PCFILEDIR_TO_PREFIX ${CMAKE_INSTALL_FULL_DATAROOTDIR}/argum ${CMAKE_INSTALL_PREFIX})
string(REGEX REPLACE "/+$" "" FROM_PCFILEDIR_TO_PREFIX "${FROM_PCFILEDIR_TO_PREFIX}") 

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/argum.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/argum.pc
    @ONLY
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/argum.pc
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)