cmake_minimum_required (VERSION 2.8.1)

set (PACKAGE_NAME "versioning")

set (ADD_DIRECTORY "module_${PACKAGE_NAME}")

set (QT_USE_QTMAIN TRUE)
set (QT_USE_SVG TRUE)
set (QT_USE_NETWORK TRUE)
set (QT_MORE_COMPONENTS ${QT_MORE_COMPONENTS} QtSvg QtNetwork)
include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/qt5.cmake )
find_package(Qt5Network)
find_package(Qt5Xml)

include_directories (
  ${CMAKE_CURRENT_LIST_DIR}/../${ADD_DIRECTORY}
  )
