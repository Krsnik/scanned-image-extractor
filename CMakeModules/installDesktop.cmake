## this macro will look for the local applications path
## and write a .desktop file

macro (INSTALL_DESKTOP_LOCAL execflags comment importerpath iconsubpath iconsize additionalText)
  if (UNIX)
    set(ICON_INSTALL "/usr/share/icons/hicolor/")

    set(dir "$ENV{HOME}/.local/share/applications/")
    IF(EXISTS ${dir})
      set(DESKTOP_FILENAME "${dir}${PROJECTNAME}.desktop")
      set(DESKTOP_FILENAME_BUILD "${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}.desktop")
      #NOTE: copying desktop file to ${DESKTOP_FILENAME} . \nThis will ensure"
      # a nice icon and possibly offer \"${PROJECTNAME}\" to the user"
      # on certain file or device events\n Even without make install." (GNOME)

      execute_process(
            COMMAND whoami
            OUTPUT_VARIABLE user OUTPUT_STRIP_TRAILING_WHITESPACE)
      if (NOT ${user} STREQUAL "root" )
          file(WRITE ${DESKTOP_FILENAME}
                "[Desktop Entry]\n"
                "Version=1.0\n"
                "Name=${PROJECTNAME}\n"
                "Comment=${comment}\n"
                "Exec=${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}${execflags}\n"
                "Icon=${CMAKE_CURRENT_SOURCE_DIR}/${iconsubpath}/${PROJECTNAME}.png\n"
                ${additionalText})
      endif()

      file(WRITE ${DESKTOP_FILENAME_BUILD}
            "[Desktop Entry]\n"
            "Version=1.0\n"
            "Name=${PROJECTNAME}\n"
            "Comment=${comment}\n"
            "Exec=${PROJECTNAME}\n"
            "Icon=${ICON_INSTALL}128x128/apps/${PROJECTNAME}.png\n"
            ${additionalText})

      set(DESKTOP_DIR "/usr/share/applications/")
      if(EXISTS ${DESKTOP_DIR})
        INSTALL(FILES
            ${DESKTOP_FILENAME_BUILD}
            DESTINATION ${DESKTOP_DIR}
            RENAME ${PROJECTNAME}.desktop
            COMPONENT main
        )
      else()
        message("Warning: not installing desktop file, ${DESKTOP_DIR} not found or changed ?")
      endif()

      if(EXISTS ${ICON_INSTALL})
        #GET_FILENAME_COMPONENT(iconSuffix ${CMAKE_CURRENT_SOURCE_DIR}/${iconsubpath} EXT)
        FOREACH (size ${iconsize})
            set(ICON_INSTALL "/usr/share/icons/hicolor/${size}/apps/")
            INSTALL(FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/${iconsubpath}/${PROJECTNAME}${size}.png"

                DESTINATION ${ICON_INSTALL}
                RENAME "${PROJECTNAME}.png"
                COMPONENT main
            )
        ENDFOREACH(size)
      else()
        message("Warning: not installing icon for unix, dir ${ICON_INSTALL} not found or changed?")
      endif()

      set(KDE_ACTIONS "/usr/share/kde4/apps/solid/actions/")
      if(EXISTS ${KDE_ACTIONS} AND NOT ${importerpath} STREQUAL "")
        INSTALL(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/${importerpath}
            DESTINATION ${KDE_ACTIONS}
            COMPONENT main
        )
      endif()
    else()
      message("applications path not found")
    endif()
  endif(UNIX)
endmacro (INSTALL_DESKTOP_LOCAL)
