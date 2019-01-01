#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2019 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(aspell MODULE
weechat-aspell.c weechat-aspell.h
weechat-aspell-bar-item.c weechat-aspell-bar-item.h
weechat-aspell-command.c weechat-aspell-command.h
weechat-aspell-completion.c weechat-aspell-completion.h
weechat-aspell-config.c weechat-aspell-config.h
weechat-aspell-info.c weechat-aspell-info.h
weechat-aspell-speller.c weechat-aspell-speller.h)
set_target_properties(aspell PROPERTIES PREFIX "")

if(ENCHANT_FOUND)
  include_directories(${ENCHANT_INCLUDE_DIR})
  target_link_libraries(aspell ${ENCHANT_LIBRARIES})
  add_definitions(-DUSE_ENCHANT)
else()
  if(ASPELL_FOUND)
    include_directories(${ASPELL_INCLUDE_PATH})
    target_link_libraries(aspell ${ASPELL_LIBRARY})
  endif()
endif()

install(TARGETS aspell LIBRARY DESTINATION ${LIBDIR}/plugins)
