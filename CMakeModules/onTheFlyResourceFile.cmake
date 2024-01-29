#  Copyright (c) 2006-2011 Peter KÃ¼mmel, <syntheti...@gmx.net>
#                2012, Kornel Benko, <kor...@lyx.org>
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  1. Redistributions of source code must retain the copyright
#         notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the copyright
#         notice, this list of conditions and the following disclaimer in the
#         documentation and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#         derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
#  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
#  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
#  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
#  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# To call this script, one has to proved following parameters
# RESOURCE_NAME             # full path of the resulting resource-file
# MAPPED_DIR                # Path-prefix to be removed from the file names

find_package (Qt5LinguistTools REQUIRED)
get_target_property(_qmake Qt5::qmake LOCATION)
execute_process( COMMAND ${_qmake} -query QT_INSTALL_TRANSLATIONS OUTPUT_VARIABLE QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE )

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

set(resource_name ${RESOURCE_NAME})

message(STATUS "Generating ${resource_name}")

file(WRITE ${resource_name} "<!DOCTYPE RCC><RCC version=\"1.0\">\n")
file(APPEND ${resource_name} "<qresource>\n")

foreach (_current_FILE ${AddQtRessourceFiles})
    get_filename_component(_abs_FILE ${_current_FILE} ABSOLUTE)
    string(REGEX REPLACE "${MAPPED_DIR}" "" _file_name ${_abs_FILE})
    file(APPEND ${resource_name} "        <file alias=\"${_file_name}\">${_abs_FILE}</file>\n")
endforeach (_current_FILE)

# add QT translation file, if available
message("Translations dir: ${QT_TRANSLATIONS_DIR}")
find_file(QT_GERMAN qt_de.qm "${QT_TRANSLATIONS_DIR}")
if (QT_GERMAN)
  file(APPEND ${resource_name} "        <file alias=\"/qt_de.qm\">${QT_TRANSLATIONS_DIR}/qt_de.qm</file>\n")
endif()
find_file(QT_BASE qtbase_de.qm "${QT_TRANSLATIONS_DIR}")
if (QT_BASE)
  file(APPEND ${resource_name} "        <file alias=\"/qtbase_de.qm\">${QT_TRANSLATIONS_DIR}/qtbase_de.qm</file>\n")
endif()
find_file(QT_GERMAN_HELP qt_help_de.qm "${QT_TRANSLATIONS_DIR}")
if (QT_GERMAN_HELP)
  file(APPEND ${resource_name} "        <file alias=\"/qt_help_de.qm\">${QT_TRANSLATIONS_DIR}/qt_help_de.qm</file>\n")
endif()
find_file(QT_GERMAN_BASE qtbase_de.qm "${QT_TRANSLATIONS_DIR}")
if(QT_GERMAN_BASE)
  file(APPEND ${resource_name} "        <file alias=\"/qtbase_de.qm\">${QT_TRANSLATIONS_DIR}/qtbase_de.qm</file>\n")
endif()

# finish ressource file
file(APPEND ${resource_name} "</qresource>\n")
file(APPEND ${resource_name} "</RCC>\n")

add_custom_target(Ressoures.qrc DEPENDS ${resource_name})
qt5_add_resources(${PROJECTNAME}_RESOURCES_RCC ${resource_name})

# add qt translation, as well
#/usr/share/qt4/translations/qt_de.qm

#QT4_ADD_RESOURCES   (${PROJECTNAME}_RESOURCES_TRANSLATIONS_RCC "${resource_name}.qrc")
