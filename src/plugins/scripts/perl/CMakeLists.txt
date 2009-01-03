# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

ADD_LIBRARY(perl MODULE weechat-perl.c weechat-perl.h weechat-perl-api.c
weechat-perl-api.h)

SET_TARGET_PROPERTIES(perl PROPERTIES PREFIX "")

IF(PERL_FOUND)
  ADD_DEFINITIONS(${PERL_CFLAGS})
  INCLUDE_DIRECTORIES(${PERL_INCLUDE_PATH})
  # ugly hack to force linking against Dynaloader.a
  STRING(REGEX MATCH "/[^ $]*/DynaLoader.a" PERL_DYNALOADER ${PERL_LFLAGS})
  IF(PERL_DYNALOADER)
    STRING(REPLACE "${PERL_DYNALOADER}" "" PERL_LFLAGS "${PERL_LFLAGS}")
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy ${PERL_DYNALOADER} ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a)
    TARGET_LINK_LIBRARIES(perl ${PERL_LIBRARY} weechat_scripts ${CMAKE_CURRENT_BINARY_DIR}/libDynaLoader.a)
  ELSE(PERL_DYNALOADER)
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PERL_LFLAGS}")
    TARGET_LINK_LIBRARIES(perl ${PERL_LIBRARY} weechat_scripts)
  ENDIF(PERL_DYNALOADER)
ENDIF(PERL_FOUND)

INSTALL(TARGETS perl LIBRARY DESTINATION lib/${PROJECT_NAME}/plugins)
