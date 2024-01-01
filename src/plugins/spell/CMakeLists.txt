#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2024 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(spell MODULE
  spell.c spell.h
  spell-bar-item.c spell-bar-item.h
  spell-command.c spell-command.h
  spell-completion.c spell-completion.h
  spell-config.c spell-config.h
  spell-info.c spell-info.h
  spell-speller.c spell-speller.h
)
set_target_properties(spell PROPERTIES PREFIX "")

if(ENCHANT_FOUND)
  include_directories(${ENCHANT_INCLUDE_DIR})
  target_link_libraries(spell ${ENCHANT_LIBRARIES} coverage_config)
  add_definitions(-DUSE_ENCHANT)
else()
  if(ASPELL_FOUND)
    include_directories(${ASPELL_INCLUDE_PATH})
    target_link_libraries(spell ${ASPELL_LIBRARY} coverage_config)
  endif()
endif()

install(TARGETS spell LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
