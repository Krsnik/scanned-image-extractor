#### translation stuff #####
# http://www.cmake.org/cmake/help/v2.8.9/cmake.html
#######

find_package(Qt5Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

SET(${PROJECTNAME}_TRANSPREFIX
    trans_${PROJECTNAME}_de
)

SET(${PROJECTNAME}_TRANSLATIONS
    ${${PROJECTNAME}_TRANSPREFIX}.ts
)

SET(${PROJECTNAME}_TRANSLATIONS_COMPILED
    ${${PROJECTNAME}_TRANSPREFIX}.qm
)

QT5_CREATE_TRANSLATION (${PROJECTNAME}_TRANSLATION_FILES
  ${${PROJECTNAME}_FORMS}
  ${${PROJECTNAME}_HEADERS}
  ${${PROJECTNAME}_SOURCES}
  ${${PROJECTNAME}_RESSOURCES}
  ${${PROJECTNAME}_TRANSLATIONS}
)

SET(AddQtRessourceFiles
    "${CMAKE_CURRENT_BINARY_DIR}/${${PROJECTNAME}_TRANSLATIONS_COMPILED}"
    ${AddQtRessourceFiles}
)

file(GLOB_RECURSE TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECTNAME}_TRANSLATIONS}")
# do not install since normally it's included in the exe file
# if(UNIX)
#     install(FILES ${TS_FILES} ${QM_FILES}
#             DESTINATION "${CMAKE_INSTALL_PREFIX}/${PROJECTNAME}/translations"
#             COMPONENT main
#     )
#     # qm is created by make, so cmake won't find it
#     # directly install it
#     install(FILES
#             "${CMAKE_CURRENT_BINARY_DIR}/${${PROJECTNAME}_TRANSLATIONS_COMPILED}"
#             DESTINATION "${CMAKE_INSTALL_PREFIX}/${PROJECTNAME}/translations"
#             COMPONENT main
#     )
# else()
#     install(FILES ${TS_FILES} ${QM_FILES}
#             DESTINATION translations
#             COMPONENT main
#     )
#     # qm is created by make, so cmake won't find it
#     # directly install it
#     install(FILES
#             "${CMAKE_CURRENT_BINARY_DIR}/${${PROJECTNAME}_TRANSLATIONS_COMPILED}"
#                     DESTINATION translations
#                     COMPONENT main
#     )
# endif()
