cmake_minimum_required (VERSION 2.8.1)



set (PACKAGE_NAME "misc")

set (ADD_DIRECTORY "module_${PACKAGE_NAME}")
include(${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/NewIfNotExists.cmake)


include_directories (
  ${CMAKE_CURRENT_LIST_DIR}/../${ADD_DIRECTORY}
  )

set ( MY_LIBS
  ${MY_LIBS}
  ${PACKAGE_NAME}
  )