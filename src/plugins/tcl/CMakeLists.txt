#
# Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
# Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2008-2024 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(tcl MODULE
  weechat-tcl.c weechat-tcl.h
  weechat-tcl-api.c weechat-tcl-api.h
)
set_target_properties(tcl PROPERTIES PREFIX "")

if(TCL_FOUND)
  set(CMAKE_REQUIRED_INCLUDES "${TCL_INCLUDE_PATH}")
  set(CMAKE_REQUIRED_LIBRARIES "${TCL_LIBRARY}")
  include_directories(${TCL_INCLUDE_PATH})
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${TCL_LFLAGS}")
  target_link_libraries(tcl ${TCL_LIBRARY} weechat_plugins_scripts coverage_config)
endif()

install(TARGETS tcl LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
