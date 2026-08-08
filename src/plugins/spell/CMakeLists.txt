# SPDX-FileCopyrightText: 2006 Emmanuel Bouthenot <kolter@openics.org>
# SPDX-FileCopyrightText: 2006-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(ENABLE_ENCHANT)
  pkg_check_modules(ENCHANT REQUIRED enchant-2)
else()
  find_package(Aspell REQUIRED)
endif()

add_library(spell MODULE
  spell.c spell.h
  spell-bar-item.c spell-bar-item.h
  spell-command.c spell-command.h
  spell-completion.c spell-completion.h
  spell-config.c spell-config.h
  spell-info.c spell-info.h
  spell-speller.c spell-speller.h
  spell-theme.c spell-theme.h
)
set_target_properties(spell PROPERTIES PREFIX "")

if(ENCHANT_FOUND)
  include_directories(${ENCHANT_INCLUDE_DIRS})
  target_link_libraries(spell ${ENCHANT_LDFLAGS} coverage_config)
  add_definitions(-DUSE_ENCHANT)
else()
  if(ASPELL_FOUND)
    include_directories(${ASPELL_INCLUDE_PATH})
    target_link_libraries(spell ${ASPELL_LIBRARY} coverage_config)
  endif()
endif()

install(TARGETS spell LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
