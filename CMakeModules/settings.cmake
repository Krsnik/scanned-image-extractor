cmake_minimum_required (VERSION 2.8.8)

ENABLE_TESTING()
#CONFIGURE_FILE(${CMAKE_CURRENT_LIST_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)

# set CMake modules path
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(filenameBITTest "${CMAKE_BINARY_DIR}/testBITs.cpp")
file(WRITE ${filenameBITTest} "
// http://stackoverflow.com/a/1505664
#include <iostream>
template<int> void DoMyOperationHelper();
template<> void DoMyOperationHelper<4>() {
  std::cout << 32;
}
template<> void DoMyOperationHelper<8>() {
  std::cout << 64;
}
// helper function just to hide clumsy syntax
inline void DoMyOperation() { DoMyOperationHelper<sizeof(size_t)>(); }
int main(){
  // appropriate function will be selected at compile time
  DoMyOperation();
  return 0;
}" )
try_run(RUN_RESULT_VAR  COMPILE_RESULT_VAR
        ${CMAKE_BINARY_DIR}  ${filenameBITTest}
          COMPILE_OUTPUT_VARIABLE comp
          RUN_OUTPUT_VARIABLE run
          )
if( "64" STREQUAL "${run}")
  if (WIN32)
    set(ARCH_DIR "win64")
  else()
    set(ARCH_DIR "unix64")
  endif()
else()
  if (WIN32)
    set(ARCH_DIR "win32")
  else()
    set(ARCH_DIR "unix32")
  endif()
endif()

message("Build architecture ${ARCH_DIR}")

# set default build type to release
# (None Debug Release RelWithDebInfo MinSizeRel)
#set (CMAKE_BUILD_TYPE "Debug")

# for debug version
#set (CMAKE_DEBUG_POSTFIX "d")

# detect svn version
# if existant

set(PATCH_NO_FILE "${CMAKE_CURRENT_LIST_DIR}/PATCH_NO")

    
    

file (STRINGS ${PATCH_NO_FILE} CURRENT_PATCH_NUMBER)
FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/patchnumber.h"
    "#ifndef DR_PATCH_NUMBER\n#define DR_PATCH_NUMBER ${CURRENT_PATCH_NUMBER}\n#endif\n" )


# save which project is currently being built.
# this allows for dynamic insertion of dependencies, if not met
# ALso two ways of building can be realized:
#   - build ALL modules, thirdparty and projects
#   - build particular module and it's dependencies, only 
#     (e.g. for windows installer)
if (DEFINED PROJECTNAME)
  ## FRESH_RUN is set per CMAKE run
  # if it is not set, the state of included dependency projects
  # is deleted
  # this enforces the same route of dependencies being taking
  # no matter if it is the first run or concecutive runs...
  #
  # together with standalone.cmake this ensures correct binding of
  # dependencies outside from the current source tree. Also this
  # ensures changes of outside source trees being committed to a
  # new buildRTYFRESH_RUN)
  if(NOT DEFINED FRESH_RUN)
    set(FRESH_RUN 1)
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/built/")
  endif(NOT DEFINED FRESH_RUN)

  # used for standalone.cmake, here save that this current project
  # is already being built
  set(BUILDFILE "${CMAKE_BINARY_DIR}/built/${PROJECTNAME}")
  IF (NOT EXISTS ${BUILDFILE})
    FILE (WRITE ${BUILDFILE} "this project has already been included for build\n")
  ENDIF (NOT EXISTS ${BUILDFILE})
endif()

 #enable std 2011 Standard for GNU
 if(NOT MSVC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                     OUTPUT_VARIABLE GCC_VERSION)
    #if (CMAKE_COMPILER_IS_GNUCC AND (GCC_VERSION VERSION_LESS 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
    #message(STATUS "Version <= 4.7")
         if ( (NOT (${PROJECTNAME} STREQUAL "libjpeg")) )
            add_definitions("-std=c++11")
        #endif()
    else()
         remove_definitions("-std=c++11")
    endif()
 endif()
#-static -static-libgcc -static-libstdc++
if (MINGW)
	# for windows, make sure the required STD libs is linked statically into the executable
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
     set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -static-libstdc++")
     set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++")
         SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++" )
         SET(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++" )
	 #SET_TARGET_PROPERTIES(${PROJECTNAME} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
endif()

# set warning levels
# http://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
endif()

if (WIN32)
  get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

  if (${ARCH_DIR} STREQUAL "win64")
    INSTALL(FILES
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
      DESTINATION bin
      COMPONENT main
      )
  endif()
  INSTALL(FILES
    "${MINGW_BIN_DIR}/libstdc++-6.dll"
    "${MINGW_BIN_DIR}/libwinpthread-1.dll"
    "${MINGW_BIN_DIR}/libgomp-1.dll"
    DESTINATION bin
    COMPONENT main
  )
endif()

#message ( "${ARCH_DIR}, SVN revision: ${CURRENT_PATCH_NUMBER}")

# ...
#link_directories(${CMAKE_BINARY_DIR}/lib)
#link_directories(${CURRENT_LIST_DIR}/../lib)

include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/${PROJECT_NAME})
include_directories(${CMAKE_BINARY_DIR}/module_${PROJECT_NAME})

set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
SET(CPACK_NSIS_CONTACT "info@dominik-ruess.de")
