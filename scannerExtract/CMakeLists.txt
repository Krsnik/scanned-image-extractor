 #######################################################################
 # This file is part of Scanned Image Extract.
 #
 # Scanned Image Extract is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # Scanned Image Extract is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with Scanned Image Extract.  If not, see <http://www.gnu.org/licenses/>
 #
 #
 # Copyright (C) 2015, Dominik Rueß; info@dominik-ruess.de
 ######################################################################/

cmake_minimum_required (VERSION 2.8)

set (PROJECTNAME "scannedImageExtractor")
project (${PROJECTNAME} CXX)

include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/settings.cmake )

find_package (versioning REQUIRED)

find_package (misc REQUIRED)

find_package (liblbfgs REQUIRED)

set (QT_USE_QTMAIN TRUE)
set (QT_USE_SVG TRUE)
set (QT_USE_NETWORK TRUE)
set (QT_MORE_COMPONENTS QtSvg QtNetwork)
include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/qt5.cmake )
find_package (Qt5Network  REQUIRED)
find_package (Qt5Svg REQUIRED)

if (OPENCV2)
    find_package (OpenCV REQUIRED core imgproc highgui)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPENCV2")
else()
    find_package (OpenCV REQUIRED core imgproc imgcodecs)
endif()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

include_directories( ${OpenCV_INCLUDE_DIRS} )

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(CMAKE_AUTOMOC ON)

include_directories (
  .
  ${CMAKE_CURRENT_BINARY_DIR}
  )

set ( ${PROJECTNAME}_SOURCES
    mainwindow.cpp 
    imageboundary.cpp 
    main.cpp 
    imagescene.cpp 
    preloadsource.cpp 
    copytargets.cpp 
    about.cpp 
    version_scannerExtract.cpp
    settingsdialog.cpp
    extracttargets.cpp
    TargetImage.cpp
    sourcefile.cpp
    helpdialog.cpp
  )

set ( ${PROJECTNAME}_HEADERS
    mainwindow.h 
    extracttargets.h
    imageboundary.h 
    imagescene.h 
    settings.h 
    sourcefile.h 
    TargetImage.h 
    preloadsource.h 
    copytargets.h 
    about.h 
    version_scannerExtract.h
    settingsdialog.h
    helpdialog.h

    )

set (${PROJECTNAME}_RESSOURCES
    scannerIcons.qrc
  )

set (${PROJECTNAME}_FORMS
    mainwindow.ui
    about.ui
    settingsdialog.ui
    helpdialog.ui
)

include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/addtranslation.cmake )

# for this application, actually build the ressource file and include it
set(RESOURCE_NAME "translations")
set(MAPPED_DIR ${CMAKE_CURRENT_BINARY_DIR})
include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/onTheFlyResourceFile.cmake )

QT5_WRAP_UI         (${PROJECTNAME}_FORMS_HEADERS ${${PROJECTNAME}_FORMS})
QT5_ADD_RESOURCES   (${PROJECTNAME}_RESOURCES_RCC ${${PROJECTNAME}_RESSOURCES})

# set exe icon  
IF(WIN32)
  IF( MINGW )
    # resource compilation for MinGW
    ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/scannerImageExtractor_ico.o
                        COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/scannerImageExtractor_ico.rc
                             -o ${CMAKE_CURRENT_BINARY_DIR}/scannerImageExtractor_ico.o )
    SET(${PROJECTNAME}_SOURCES ${${PROJECTNAME}_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/scannerImageExtractor_ico.o)
  ELSE( MINGW )
    SET(${PROJECTNAME}_SOURCES ${${PROJECTNAME}_SOURCES} scannerImageExtractor_ico.rc)
  ENDIF( MINGW )
ENDIF(WIN32)

add_executable (${PROJECTNAME} WIN32
  ${${PROJECTNAME}_HEADERS}
  ${${PROJECTNAME}_SOURCES}
  ${${PROJECTNAME}_HEADERS_MOC}
  ${${PROJECTNAME}_FORMS_HEADERS}
  ${${PROJECTNAME}_RESOURCES_RCC}
  ${${PROJECTNAME}_TRANSLATION_FILES}
  )

qt5_use_modules (${PROJECTNAME} Network Widgets Svg)
TARGET_LINK_LIBRARIES (${PROJECTNAME}
  ${MY_LIBS}
  ${QT_LIBRARIES}
  ${OpenCV_LIBS}
  ${LIBLBFGS_LIBRARY}
  )

if(WIN32 OR MINGW)
TARGET_LINK_LIBRARIES (${PROJECTNAME}
  Qt5::WinMain
  )
endif()

install(TARGETS ${PROJECTNAME}
  RUNTIME DESTINATION bin COMPONENT main
  )

if (WIN32)
    INSTALL(FILES
            ${LIBLBFGS_LIBRARY_DLL}
	${OpenCV_DIR}/bin/libopencv_core300.dll
	${OpenCV_DIR}/bin/libopencv_imgproc300.dll
	${OpenCV_DIR}/bin/libopencv_imgcodecs300.dll
      DESTINATION bin
      COMPONENT main
      )
endif(WIN32)

# add nice icon for Gnome and KDE and add to default programs for cameras
include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/installDesktop.cmake )
#set(sizes "16x16" "22x22" "24x24" "36x36" "42x42" "72x72" "96x96" "32x32" "48x48" "64x64" "128x128" "80x80")
set(sizes "128x128" "256x256")
INSTALL_DESKTOP_LOCAL(" %f"
                      "Scanned Image Extractor"
                      ""
                      "ico"
                      "${sizes}"
                      "Terminal=false
Type=Application
MimeType=x-content/image-dcf;
Categories=Graphics\;Photography\;GNOME\;KDE\;
X-GNOME-FullName=Scanned Image Extractor\n")

include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/installQt5.cmake )

set ( PACKAGE_NAME ${PROJECTNAME} )
set ( MAIN_DISPLAY_NAME "Scanned Image Extractor")
set ( USE_COMPONENTS main )
SET ( VERSION_MAJOR 0 )
SET ( VERSION_MINOR 2 )
set ( VERSION_PATCH ${CURRENT_PATCH_NUMBER})

IF(WIN32)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\logo.ico")
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\logo.png")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\scannedImageExtractor.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}Scanned Image Extractor")
Endif(WIN32)

IF(UNIX)
  message("using ${CMAKE_SYSTEM_PROCESSOR}")
 # http://www.cmake.org/Wiki/CMake:CPackPackageGenerators#DEB_.28UNIX_only.29
    SET(CPACK_SET_DESTDIR "on")
    SET(CPACK_DEBIAN_PACKAGE_NAME "scannedImageExtractor")
    SET(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
  if(MAKE_RPM)
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST ${CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST}
    	/usr/local
	/usr/local/bin
	/usr
	/usr/share
	/usr/share/applications
	/usr/share/icons
	/usr/share/icons/hicolor
	/usr/share/icons/hicolor/256x256/apps
	/usr/share/icons/hicolor/256x256
	/usr/share/icons/hicolor
	/usr/share/icons/hicolor/128x128
	/usr/share/icons/hicolor/128x128/apps
	/usr/share/icons/hicolor/256x256/apps
	/usr/share/icons/hicolor/256x256)
    SET(CPACK_GENERATOR "RPM")
    SET(CPACK_RPM_PACKAGE_LICENSE "GPL v3")
    SET(CPACK_RPM_PACKAGE_VENDOR "Dominik Rueß <scannerExtract@dominik-ruess.de>")
    SET(CPACK_RPM_PACKAGE_REQUIRES "opencv-core >= 2.4, qt5-qtsvg >= 5.0, qt5-qtbase >= 5.0,  liblbfgs >= 1.0")
    SET(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
  else()
    SET(CPACK_GENERATOR "DEB")
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5network5, libqt5gui5, libqt5svg5, libqt5widgets5, liblbfgs0, libopencv-core2.4, libopencv-highgui2.4, libopencv-imgproc2.4")
    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "kde")
    SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    SET(CPACK_PACKAGE_CONTACT "scannerExtract@dominik-ruess.de")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dominik Rueß <scannerExtract@dominik-ruess.de>")
  endif()
ENDIF()


include ( ${CMAKE_CURRENT_LIST_DIR}/../CMakeModules/buildPackage.cmake )
