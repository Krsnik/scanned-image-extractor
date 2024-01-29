cmake_minimum_required (VERSION 2.8.8)


set (PROJECTNAME "misc")
project (${PROJECTNAME} CXX)

include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/settings.cmake )


include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/qt5.cmake )


find_package (versioning REQUIRED)


include_directories (
  .
  )

set ( ${PROJECTNAME}_SOURCES
    filenamenumbering_pre.cpp
    version_module_misc.cpp
    translation.cpp
  )

set ( ${PROJECTNAME}_HEADERS
    filenamenumbering_pre.h
    version_module_misc.h
    translation.h
  )

set (${PROJECTNAME}_RESSOURCES
  )

set (${PROJECTNAME}_FORMS
)

add_library(${PROJECTNAME} STATIC
  ${${PROJECTNAME}_SOURCES}
  ${${PROJECTNAME}_HEADERS}
  ${${PROJECTNAME}_RESSOURCES}
  ${${PROJECTNAME}_FORMS}
)

qt5_use_modules(${PROJECTNAME} Core Widgets Gui)


TARGET_LINK_LIBRARIES (${PROJECTNAME}
  ${MY_LIBS}
  ${QT_LIBRARIES}
  )


