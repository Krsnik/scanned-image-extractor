#
# Try to find the Liblbfgs library and include path.
# Once done this will define
#
# LIBLBFGS_FOUND
# LIBLBFGS_INCLUDE_PATH
# LIBLBFGS_LIBRARY

if(MAKE_RPM)

  FIND_PATH( LIBLBFGS_INCLUDE_PATH lbfgs.h
    PATHS ${MANUAL_LBFGS}/include
    DOC "The directory where lbfgs.h resides"
    
          [NO_DEFAULT_PATH]
          [NO_CMAKE_ENVIRONMENT_PATH]
          [NO_CMAKE_PATH]
          [NO_SYSTEM_ENVIRONMENT_PATH]
          [NO_CMAKE_SYSTEM_PATH])

  FIND_LIBRARY( LIBLBFGS_LIBRARY
    NAMES liblbfgs lbfgs
    PATHS ${MANUAL_LBFGS}/lib/.libs
     ${MANUAL_LBFGS}/lib
    DOC "The Lbfgs library"     
              NO_DEFAULT_PATH
          NO_CMAKE_ENVIRONMENT_PATH
          NO_CMAKE_PATH
          NO_SYSTEM_ENVIRONMENT_PATH
          NO_CMAKE_SYSTEM_PATH)
    
else(MAKE_RPM)
  FIND_PATH( LIBLBFGS_INCLUDE_PATH lbfgs.h
  PATHS
    /usr/include
    /usr/include/lbfgs
    /usr/local/include
    /usr/local/include/lbfgs
    /sw/include
    /sw/include/lbfgs
    /opt/local/include
    /opt/local/include/lbfgs
    DOC "The directory where lbfgs.h resides")

  FIND_LIBRARY( LIBLBFGS_LIBRARY
    NAMES liblbfgs lbfgs
    PATHS
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /sw/lib
    /opt/local/lib
    DOC "The Lbfgs library")
endif(MAKE_RPM)

if(WIN32)
  FIND_LIBRARY( LIBLBFGS_LIBRARY_DLL
    NAMES liblbfgs-1-10.dll
    PATHS
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /sw/lib
    /opt/local/lib
    DOC "The Lbfgs library")

    IF (LIBLBFGS_INCLUDE_PATH AND LIBLBFGS_LIBRARY AND LIBLBFGS_LIBRARY_DLL)
      SET( LIBLBFGS_FOUND TRUE CACHE BOOL "Set to TRUE if liblbfgs is found, FALSE otherwise")
    ENDIF (LIBLBFGS_INCLUDE_PATH AND LIBLBFGS_LIBRARY  AND LIBLBFGS_LIBRARY_DLL)
    MARK_AS_ADVANCED(
            LIBLBFGS_FOUND
            LIBLBFGS_LIBRARY
            LIBLBFGS_LIBRARY_DLL
            LIBLBFGS_INCLUDE_PATH)
else()

    IF (LIBLBFGS_INCLUDE_PATH AND LIBLBFGS_LIBRARY)
      SET( LIBLBFGS_FOUND TRUE CACHE BOOL "Set to TRUE if liblbfgs is found, FALSE otherwise")
    ENDIF (LIBLBFGS_INCLUDE_PATH AND LIBLBFGS_LIBRARY)
    MARK_AS_ADVANCED(
            LIBLBFGS_FOUND
            LIBLBFGS_LIBRARY
            LIBLBFGS_INCLUDE_PATH)
endif()

if(UNIX AND NOT LIBLBFGS_FOUND)
  message(WARNING "LIBLBFGS not found")
elseif(UNIX)
  set ( MY_LIBS
    ${MY_LIBS}
    ${LIBLBFGS_LIBRARY}
    )

  if (NOT ${LIBLBFGS_INCLUDE_PATH} STREQUAL "/usr/include/lbfgs")
    include_directories (
      "${LIBLBFGS_INCLUDE_PATH}/../"
      "${LIBLBFGS_INCLUDE_PATH}")
  endif()
else()
    include_directories (
      "${LIBLBFGS_INCLUDE_PATH}"
      )
  set ( MY_LIBS
    ${MY_LIBS}
    ${LIBLBFGS_LIBRARY_DLL}
    )
endif(UNIX AND NOT LIBLBFGS_FOUND)

