# will include modules/thirdparty if needed for out of source building
# make sure PROJECTNAME and PACKAGE_NAME are set properly

set(BUILDFILE "${CMAKE_BINARY_DIR}/built/${PACKAGE_NAME}")

# has it been built yet ?
if(NOT EXISTS "${BUILDFILE}")
  set(FROMDIR ${CMAKE_CURRENT_LIST_DIR}/../${ADD_DIRECTORY})
  set(TODIR ${CMAKE_BINARY_DIR}/${SUBDIR_STANDALONE}/${ADD_DIRECTORY})

  # projectname must not equal package name,
  # this happens if projectname finds itself, for instance
  if(NOT ${PROJECTNAME} STREQUAL ${PACKAGE_NAME} AND NOT EXISTS "${BUILDFILE}")
    add_subdirectory (${FROMDIR}  ${TODIR})
  endif()
endif()

# # add as target, if it does not exist already
# # this makes  it possible to separately
# # compile different programs

# set(DEST_DIR_TMP "${CMAKE_BINARY_DIR}/${ADD_DIRECTORY}")
# if (NOT ${PROJECTNAME} STREQUAL ${PACKAGE_NAME})
#   if (NOT TARGET ${PACKAGE_NAME})
#     If(NOT IS_DIRECTORY ${DEST_DIR_TMP})
#       set(TRGTDIR "${CMAKE_BINARY_DIR}/${ADD_DIRECTORY}")
#       add_subdirectory (
#         "${CMAKE_CURRENT_LIST_DIR}/../${ADD_DIRECTORY}"
#         ${TRGTDIR}
#       )
#       set (LINK_DIRECTORIES ${TRGTDIR} ${LINK_DIRECTORIES})
#     endif()
#   endif()
# endif()

