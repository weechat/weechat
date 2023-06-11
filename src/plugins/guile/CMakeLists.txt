#
# Copyright (C) 2011-2023 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(guile MODULE
  weechat-guile.c weechat-guile.h
  weechat-guile-api.c weechat-guile-api.h
)
set_target_properties(guile PROPERTIES PREFIX "")

if(GUILE_FOUND)
  include_directories(${GUILE_INCLUDE_DIRS})
  set(LINK_LIBS)
  list(APPEND LINK_LIBS ${GUILE_LDFLAGS})
  target_link_libraries(guile ${LINK_LIBS} weechat_plugins_scripts coverage_config)
endif()

install(TARGETS guile LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
