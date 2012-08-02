#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

ADD_LIBRARY(perl MODULE weechat-perl.c weechat-perl.h weechat-perl-api.c
weechat-perl-api.h)

SET_TARGET_PROPERTIES(perl PROPERTIES PREFIX "")

IF(PERL_FOUND)
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # remove "-arch xxx" on Mac OS X
    STRING(REGEX REPLACE "-arch ppc|-arch i386|-arch x86_64" "" PERL_CFLAGS "${PERL_CFLAGS}")
    STRING(REGEX REPLACE "-arch ppc|-arch i386|-arch x86_64" "" PERL_LFLAGS "${PERL_LFLAGS}")
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ADD_DEFINITIONS(${PERL_CFLAGS})
  INCLUDE_DIRECTORIES(${PERL_INCLUDE_PATH})
  # ugly hack to force linking against Dynaloader.a
  STRING(REGEX MATCH "/[^ $]*/DynaLoader.a" PERL_DYNALOADER ${PERL_LFLAGS})
  IF(PERL_DYNALOADER)
    STRING(REPLACE "${PERL_DYNALOADER}" "" PERL_LFLAGS "${PERL_LFLAGS}")
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy ${PERL_DYNALOADER} ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a)
    TARGET_LINK_LIBRARIES(perl ${PERL_LIBRARY} weechat_plugins_scripts ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a)
  ELSE(PERL_DYNALOADER)
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    TARGET_LINK_LIBRARIES(perl ${PERL_LIBRARY} weechat_plugins_scripts)
  ENDIF(PERL_DYNALOADER)
ENDIF(PERL_FOUND)

INSTALL(TARGETS perl LIBRARY DESTINATION ${LIBDIR}/plugins)
