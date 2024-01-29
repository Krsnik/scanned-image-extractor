set (QT_VERSION_MAJOR 5)

IF(WIN32)
  if(QT_USE_SVG)
    get_target_property(QtSvg_location Qt5::Svg LOCATION_Release)
    #find_file(qsvgicon qsvgicon${QT_VERSION_MAJOR}.dll PATHS ${QT_PLUGINS_DIR}/iconengines NO_DEFAULT_PATH)
    #find_file(qsvg qsvg${QT_VERSION_MAJOR}.dll PATHS ${QT_PLUGINS_DIR}/iconengines NO_DEFAULT_PATH)
    #install(FILES ${qsvgicon}
    #  DESTINATION bin/iconengines
    #  COMPONENT main
    #  )
    INSTALL(FILES
      "${QtSvg_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()
    
  get_target_property(_loc Qt5::QWindowsIntegrationPlugin LOCATION)  
  get_filename_component(_loc_iconengine ${_loc} PATH)
  get_filename_component(_loc_iconengine "${_loc_iconengine}/../iconengines/" ABSOLUTE)
  
  file(GLOB PLUGINS_ICONENGINE "${_loc_iconengine}/*[^d].dll")
    INSTALL(FILES
      ${PLUGINS_ICONENGINE}
      DESTINATION bin/iconengines
      COMPONENT main
      )

  get_target_property(_loc Qt5::QWindowsIntegrationPlugin LOCATION)  
  get_filename_component(_loc_platform ${_loc} PATH)
  file(GLOB PLUGINS_PLATFORM "${_loc_platform}/*[^d].dll")
    INSTALL(FILES
      ${PLUGINS_PLATFORM}
      DESTINATION bin/platforms
      COMPONENT main
      )
      
  get_target_property(_loc_gif Qt5::QGifPlugin LOCATION)  
  #message(${_loc_gif})
  get_filename_component(_loc_image_plugins ${_loc_gif} PATH)
  file(GLOB PLUGINS_IMAGE "${_loc_image_plugins}/*[^d].dll")
    INSTALL(FILES
      ${PLUGINS_IMAGE}
      DESTINATION bin/imageformats
      COMPONENT main
      )


  if (QT_USE_NETWORK)
    get_target_property(QtNetwork_location Qt5::Network LOCATION_Release)
    INSTALL(FILES
      "${QtNetwork_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()

  if (QT_USE_XML)
    get_target_property(QtXml_location Qt5::Xml LOCATION_Release)
    INSTALL(FILES
      "${QtXml_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()
  
  if (QT_USE_MULTIMEDIA)
    get_target_property(QtMultimedia_location Qt5::Multimedia LOCATION_Release)
    INSTALL(FILES
      "${QtMultimedia_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()
  
  if (QT_USE_MULTIMEDIA_WIDGETS)
    get_target_property(QtMultimediaWidgets_location Qt5::MultimediaWidgets LOCATION_Release)
    INSTALL(FILES
      "${QtMultimediaWidgets_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()

  if (QT_USE_SQL)
    get_target_property(QtSql_location Qt5::Sql LOCATION_Release)
    INSTALL(FILES
      "${QtSql_location}"
      DESTINATION bin
      COMPONENT main
      )
  endif()

  # base files always
  get_target_property(QtCore_location Qt5::Core LOCATION_Release)
  get_target_property(QtWidgets_location Qt5::Widgets LOCATION_Release)
  get_target_property(QtGui_location Qt5::Gui LOCATION_Release)
  # find_package(Qt5OpenGL REQUIRED)
  #GET_TARGET_PROPERTY(QtOpenGL_location Qt5::OpenGL LOCATION_Release)
  INSTALL(FILES
    "${QtCore_location}"
    "${QtWidgets_location}"
    "${QtGui_location}"
    #"${QtOpenGL_location}"
    DESTINATION bin
    COMPONENT main
    )

  # include required DLLs, based on the assumption that these
  # are within the Qt5 bin dir. This is the case if you download
  # the precompiled minGW version of Qt5 (tested with 5.2)
  get_filename_component(QT5LOC ${QtCore_location} PATH)

  file(GLOB DLL_FILES1 "${QT5LOC}/libgcc_s_dw*.dll")
  INSTALL(FILES ${DLL_FILES1}
        DESTINATION bin
                COMPONENT main
    )


  file(GLOB DLL_FILES2 "${QT5LOC}/libstdc++*.dll")
  INSTALL(FILES ${DLL_FILES2}
        DESTINATION bin
                COMPONENT main
    )

  file(GLOB DLL_FILES3 "${QT5LOC}/libwinpthread*.dll")
  INSTALL(FILES ${DLL_FILES3}
        DESTINATION bin
                COMPONENT main
    )

  file(GLOB DLL_FILES4 "${QT5LOC}/icu*.dll")
  INSTALL(FILES ${DLL_FILES4}
        DESTINATION bin
                COMPONENT main
    )
   
  if (${ARCH_DIR} STREQUAL "win32")   
    INSTALL(FILES "${QT5LOC}/libEGL.dll" "${QT5LOC}/libGLESv2.dll"
         DESTINATION bin
                  COMPONENT main
     )
  endif()

ENDIF(WIN32)
