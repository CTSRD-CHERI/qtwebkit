# Finds the International Components for Unicode (ICU) Library
#
#  ICU_FOUND          - True if ICU found.
#  ICU_I18N_FOUND     - True if ICU's internationalization library found.
#  ICU_INCLUDE_DIRS   - Directory to include to get ICU headers
#                       Note: always include ICU headers as, e.g.,
#                       unicode/utypes.h
#  ICU_LIBRARIES      - Libraries to link against for the common ICU
#  ICU_I18N_LIBRARIES - Libraries to link against for ICU internationaliation
#                       (note: in addition to ICU_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_ICU icu-uc IMPORTED_TARGET)

# Look for the header file.
find_path(
    ICU_INCLUDE_DIR
    NAMES unicode/utypes.h
    HINTS ${PC_ICU_INCLUDE_DIRS}
          ${PC_ICU_INCLUDEDIR}
    DOC "Include directory for the ICU library")
mark_as_advanced(ICU_INCLUDE_DIR)

if(TARGET PkgConfig::PC_ICU)
  set(ICU_LIBRARY PkgConfig::PC_ICU)
else()
    # Look for the library.
    find_library(
        ICU_LIBRARY
        NAMES icuuc cygicuuc cygicuuc32
        HINTS ${PC_ICU_LIBRARY_DIRS}
              ${PC_ICU_LIBDIR}
        DOC "Libraries to link against for the common parts of ICU")
endif()
mark_as_advanced(ICU_LIBRARY)
message(STATUS "ICU_LIBRARY: ${ICU_LIBRARY}")

# Copy the results to the output variables.
if (ICU_INCLUDE_DIR AND ICU_LIBRARY)
    set(ICU_FOUND 1)
    set(ICU_LIBRARIES ${ICU_LIBRARY})
    set(ICU_INCLUDE_DIRS ${ICU_INCLUDE_DIR})

    set(ICU_VERSION 0)
    set(ICU_MAJOR_VERSION 0)
    set(ICU_MINOR_VERSION 0)
    file(READ "${ICU_INCLUDE_DIR}/unicode/uversion.h" _ICU_VERSION_CONENTS)
    string(REGEX REPLACE ".*#define U_ICU_VERSION_MAJOR_NUM ([0-9]+).*" "\\1" ICU_MAJOR_VERSION "${_ICU_VERSION_CONENTS}")
    string(REGEX REPLACE ".*#define U_ICU_VERSION_MINOR_NUM ([0-9]+).*" "\\1" ICU_MINOR_VERSION "${_ICU_VERSION_CONENTS}")

    set(ICU_VERSION "${ICU_MAJOR_VERSION}.${ICU_MINOR_VERSION}")

    # Look for the ICU internationalization libraries
    pkg_check_modules(PC_ICU_I18N icu-i18n)
    if(TARGET PkgConfig::PC_ICU_I18N)
        set(ICU_I18N_LIBRARY PkgConfig::PC_ICU_I18N)
    else()
        find_library(
            ICU_I18N_LIBRARY
            NAMES icui18n icuin cygicuin cygicuin32
            HINTS ${PC_ICU_I18N_LIBRARY_DIRS}
                  ${PC_ICU_I18N_LIBDIR}
            DOC "Libraries to link against for ICU internationalization")
    endif()
    mark_as_advanced(ICU_I18N_LIBRARY)
    if (ICU_I18N_LIBRARY)
        set(ICU_I18N_FOUND 1)
        set(ICU_I18N_LIBRARIES ${ICU_I18N_LIBRARY})
    else ()
        set(ICU_I18N_FOUND 0)
        set(ICU_I18N_LIBRARIES)
    endif ()
else ()
    set(ICU_FOUND 0)
    set(ICU_I18N_FOUND 0)
    set(ICU_LIBRARIES)
    set(ICU_I18N_LIBRARIES)
    set(ICU_INCLUDE_DIRS)
    set(ICU_VERSION)
    set(ICU_MAJOR_VERSION)
    set(ICU_MINOR_VERSION)
endif ()

if (ICU_FOUND)
    if (NOT ICU_FIND_QUIETLY)
        message(STATUS "Found ICU header files in ${ICU_INCLUDE_DIRS}")
        message(STATUS "Found ICU libraries: ${ICU_LIBRARIES}")
    endif ()
else ()
    if (ICU_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find ICU")
    else ()
        message(STATUS "Optional package ICU was not found")
    endif ()
endif ()
