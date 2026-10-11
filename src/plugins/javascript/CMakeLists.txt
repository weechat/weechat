# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

enable_language(CXX)

find_package(V8 11.3 REQUIRED)

add_library(javascript MODULE
  weechat-js.cpp weechat-js.h
  weechat-js-v8.cpp weechat-js-v8.h
  weechat-js-api.cpp weechat-js-api.h
)
set_target_properties(javascript PROPERTIES
  PREFIX ""
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

# Standalone V8 on macOS (Homebrew) is linked with "@rpath".
if(APPLE)
  set_target_properties(javascript PROPERTIES
    BUILD_RPATH "${V8_LIBRARY_DIR}"
    INSTALL_RPATH "${V8_LIBRARY_DIR}"
  )
endif()

target_compile_definitions(javascript PRIVATE ${V8_DEFINITIONS})
target_include_directories(javascript SYSTEM PRIVATE ${V8_INCLUDE_DIR})
target_link_libraries(javascript ${V8_LIBRARIES} ${CMAKE_DL_LIBS} weechat_plugins_scripts coverage_config)

install(TARGETS javascript LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
