# Copyright: 2010 RobotCub Consortium
# Author: Ilaria Gori, Sean Ryan Fanello
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
cmake_minimum_required(VERSION 2.8)

set(PROJECTNAME gesture-recognition)
project(${PROJECTNAME})

find_package(YARP REQUIRED)
find_package(ICUB REQUIRED)
find_package(ICUBcontrib REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH}
                              ${ICUB_MODULE_PATH}
                              ${ICUBCONTRIB_MODULE_PATH})
	
include(YarpInstallationHelpers) 	
include(ICUBcontribHelpers)
include(ICUBcontribOptions)
icubcontrib_set_default_prefix()

add_definitions(${YARP_DEFINES})

add_subdirectory(modules)

########## application
file(GLOB conf ${PROJECT_SOURCE_DIR}/app/conf/*.ini)
yarp_install(FILES ${conf} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME})

file(GLOB scripts ${PROJECT_SOURCE_DIR}/app/scripts/*.template ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)
set(lua_fsm       ${PROJECT_SOURCE_DIR}/app/LUA/demoRec_funcs.lua
                  ${PROJECT_SOURCE_DIR}/app/LUA/demoRec_fsm.lua
                  ${PROJECT_SOURCE_DIR}/app/LUA/demoRec_root_fsm.lua)
set(lua_bin       ${PROJECT_SOURCE_DIR}/app/LUA/demoRec_main.lua)

yarp_install(FILES    ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})
yarp_install(PROGRAMS ${lua_fsm} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${PROJECTNAME}/LUA)

add_custom_target(copy_lua_in_build ALL)
add_custom_command(TARGET copy_lua_in_build POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${lua_bin} ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
                   COMMENT "Copying ${lua_bin} to ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/")
install(PROGRAMS ${lua_bin} DESTINATION bin)

icubcontrib_add_uninstall_target()

