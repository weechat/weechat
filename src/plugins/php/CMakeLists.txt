# SPDX-FileCopyrightText: 2006-2017 Adam Saponara <as@php.net>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PHP REQUIRED)

add_library(php MODULE
  weechat-php.c weechat-php.h
  weechat-php-api.c weechat-php-api.h
)
set_target_properties(php PROPERTIES PREFIX "")

include_directories(${PHP_INCLUDE_DIRS})
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PHP_LDFLAGS}")
target_link_libraries(php ${PHP_LIB} weechat_plugins_scripts coverage_config)

install(TARGETS php LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
