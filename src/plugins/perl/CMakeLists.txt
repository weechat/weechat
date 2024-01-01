#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

add_library(perl MODULE
  weechat-perl.c weechat-perl.h
  weechat-perl-api.c weechat-perl-api.h
)
set_target_properties(perl PROPERTIES PREFIX "")

if(PERL_FOUND)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # remove "-arch xxx" on macOS
    STRING(REGEX REPLACE "-arch ppc|-arch i386|-arch x86_64" "" PERL_CFLAGS "${PERL_CFLAGS}")
    STRING(REGEX REPLACE "-arch ppc|-arch i386|-arch x86_64" "" PERL_LFLAGS "${PERL_LFLAGS}")
  endif()
  add_definitions(${PERL_CFLAGS})
  include_directories(${PERL_INCLUDE_PATH})
  # ugly hack to force linking against Dynaloader.a
  string(REGEX MATCH "/[^ $]*/DynaLoader.a" PERL_DYNALOADER ${PERL_LFLAGS})
  if(PERL_DYNALOADER)
    string(REPLACE "${PERL_DYNALOADER}" "" PERL_LFLAGS "${PERL_LFLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${PERL_DYNALOADER} ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a)
    target_link_libraries(perl ${PERL_LIBRARY} weechat_plugins_scripts ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a coverage_config)
  else()
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    target_link_libraries(perl ${PERL_LIBRARY} weechat_plugins_scripts coverage_config)
  endif()
endif()

install(TARGETS perl LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
