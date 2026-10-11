# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find V8 (Google's JavaScript engine)
# This module finds if V8 is installed and determines where the include files
# and libraries are.
#
# V8 is searched first in Node.js shared library (libnode), then as a
# standalone library (libv8 and libv8_libplatform).
#
# This code sets the following variables:
#
#  V8_FOUND       = V8 is installed
#  V8_VERSION     = V8 version (for example "13.6.233.10")
#  V8_INCLUDE_DIR = path to where v8.h can be found
#  V8_LIBRARIES   = libraries to link with
#  V8_LIBRARY_DIR = directory with V8 libraries (symbolic links resolved)
#  V8_DEFINITIONS = definitions required to compile with V8 headers
#                   (they must match the build configuration of V8)

if(V8_FOUND)
  # Already in cache, be silent.
  set(V8_FIND_QUIETLY TRUE)
endif()

# Headers are searched only in the prefix of the library found, because
# another V8 or Node.js version could be installed elsewhere (for example
# Node.js headers without library in /usr/local).
macro(v8_find_include_dir library)
  get_filename_component(V8_LIBRARY_PARENT_DIR "${library}" DIRECTORY)
  find_path(V8_INCLUDE_DIR v8.h
    PATHS "${V8_LIBRARY_PARENT_DIR}/../include" "${V8_LIBRARY_PARENT_DIR}/../../include"
    PATH_SUFFIXES ${ARGN}
    NO_DEFAULT_PATH
  )
endmacro()

# V8 provided by Node.js (libnode).
find_library(V8_NODE_LIBRARY NAMES node)
if(V8_NODE_LIBRARY)
  v8_find_include_dir("${V8_NODE_LIBRARY}" node)
  set(V8_LIBRARIES ${V8_NODE_LIBRARY})
endif()

# Standalone V8 (libv8 + libv8_libplatform), for example from Homebrew.
if(NOT V8_INCLUDE_DIR)
  set(V8_STANDALONE_PATHS /opt/homebrew/opt/v8 /usr/local/opt/v8)
  find_library(V8_LIBRARY NAMES v8
    PATHS ${V8_STANDALONE_PATHS} PATH_SUFFIXES lib)
  find_library(V8_LIBPLATFORM_LIBRARY NAMES v8_libplatform
    PATHS ${V8_STANDALONE_PATHS} PATH_SUFFIXES lib)
  if(V8_LIBRARY AND V8_LIBPLATFORM_LIBRARY)
    v8_find_include_dir("${V8_LIBRARY}" v8)
    set(V8_LIBRARIES ${V8_LIBRARY} ${V8_LIBPLATFORM_LIBRARY})
  endif()
endif()

if(V8_LIBRARIES)
  list(GET V8_LIBRARIES 0 V8_FIRST_LIBRARY)
  get_filename_component(V8_LIBRARY_DIR "${V8_FIRST_LIBRARY}" REALPATH)
  get_filename_component(V8_LIBRARY_DIR "${V8_LIBRARY_DIR}" DIRECTORY)
endif()

if(V8_INCLUDE_DIR AND EXISTS "${V8_INCLUDE_DIR}/v8-version.h")
  file(STRINGS "${V8_INCLUDE_DIR}/v8-version.h" V8_VERSION_DEFINES
    REGEX "^#define V8_(MAJOR|MINOR)_VERSION|^#define V8_(BUILD_NUMBER|PATCH_LEVEL)")
  foreach(part MAJOR_VERSION MINOR_VERSION BUILD_NUMBER PATCH_LEVEL)
    string(REGEX MATCH "#define V8_${part} +[0-9]+" V8_${part} "${V8_VERSION_DEFINES}")
    string(REGEX REPLACE "#define V8_${part} +" "" V8_${part} "${V8_${part}}")
  endforeach()
  set(V8_VERSION "${V8_MAJOR_VERSION}.${V8_MINOR_VERSION}.${V8_BUILD_NUMBER}.${V8_PATCH_LEVEL}")
endif()

# Find the definitions required to use V8 headers (pointer compression, sandbox):
# they must match the build configuration of V8, which is checked by
# V8::Initialize (the program aborts in case of mismatch).
# V8 is considered as not found if a program using V8 can not be compiled,
# linked and executed (for example if headers and library do not match).
if(DEFINED V8_DEFINITIONS)
  set(V8_CHECK_OK TRUE)
elseif(V8_INCLUDE_DIR AND V8_LIBRARIES)
  if(CMAKE_CROSSCOMPILING)
    set(V8_DEFINITIONS "" CACHE STRING "Definitions required to compile with V8 headers")
    set(V8_CHECK_OK TRUE)
  else()
    set(V8_CHECK_DIR "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckV8")
    file(WRITE "${V8_CHECK_DIR}/check_v8.cpp"
      "#include <libplatform/libplatform.h>\n"
      "#include <v8.h>\n"
      "int main ()\n"
      "{\n"
      "    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();\n"
      "    v8::V8::InitializePlatform(platform.get());\n"
      "    return (v8::V8::Initialize()) ? 0 : 1;\n"
      "}\n")
    set(V8_CHECK_LINK_OPTIONS)
    if(APPLE)
      set(V8_CHECK_LINK_OPTIONS "-Wl,-rpath,${V8_LIBRARY_DIR}")
    endif()
    set(V8_CONFIG_1 "")
    set(V8_CONFIG_2 "V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH")
    set(V8_CONFIG_3 "V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_ENABLE_SANDBOX")
    set(V8_CONFIG_4 "V8_31BIT_SMIS_ON_64BIT_ARCH")
    foreach(config 1 2 3 4)
      set(V8_CHECK_DEFINITIONS)
      foreach(definition ${V8_CONFIG_${config}})
        list(APPEND V8_CHECK_DEFINITIONS "-D${definition}")
      endforeach()
      try_run(V8_CHECK_RUN V8_CHECK_COMPILE
        "${V8_CHECK_DIR}" "${V8_CHECK_DIR}/check_v8.cpp"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${V8_INCLUDE_DIR}"
        COMPILE_DEFINITIONS ${V8_CHECK_DEFINITIONS}
        LINK_OPTIONS ${V8_CHECK_LINK_OPTIONS}
        LINK_LIBRARIES ${V8_LIBRARIES}
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
      )
      if(V8_CHECK_COMPILE AND V8_CHECK_RUN EQUAL 0)
        set(V8_DEFINITIONS "${V8_CONFIG_${config}}" CACHE STRING "Definitions required to compile with V8 headers")
        set(V8_CHECK_OK TRUE)
        break()
      endif()
    endforeach()
    if(NOT V8_CHECK_OK)
      message(STATUS "Unable to compile and run a program with V8 "
        "(headers: ${V8_INCLUDE_DIR}, libraries: ${V8_LIBRARIES}), "
        "set V8_DEFINITIONS to skip this check")
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8
  REQUIRED_VARS V8_LIBRARIES V8_INCLUDE_DIR V8_CHECK_OK
  VERSION_VAR V8_VERSION
)

mark_as_advanced(
  V8_INCLUDE_DIR
  V8_NODE_LIBRARY
  V8_LIBRARY
  V8_LIBPLATFORM_LIBRARY
  V8_DEFINITIONS
)
