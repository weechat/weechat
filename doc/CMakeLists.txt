#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

if(ENABLE_MAN OR ENABLE_DOC)

  find_package(Sourcehighlight)
  find_package(Asciidoc)

  if(ASCIIDOC_FOUND)

    # copy asciidoc.css in build dir
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/doc/asciidoc_toc.css
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.css asciidoc_toc.css
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.css
      ${CMAKE_CURRENT_SOURCE_DIR}/toc2.css
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building asciidoc_toc.css"
      )
    add_custom_target(asciidoc-toc-css ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/asciidoc_toc.css)

    # concat asciidoc.css + toc2.css into asciidoc_toc2.css (in build dir)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/doc/asciidoc_toc2.css
      COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.css ${CMAKE_CURRENT_SOURCE_DIR}/toc2.css > asciidoc_toc2.css
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.css
      ${CMAKE_CURRENT_SOURCE_DIR}/toc2.css
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building asciidoc_toc2.css"
      )
    add_custom_target(asciidoc-toc2-css ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/asciidoc_toc2.css)

    set(CSS_TOC -a stylesheet=${CMAKE_CURRENT_BINARY_DIR}/asciidoc_toc.css)
    set(CSS_TOC2 -a stylesheet=${CMAKE_CURRENT_BINARY_DIR}/asciidoc_toc2.css)

    set(ASCIIDOC_ARGS -a revision="${VERSION}" -f ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.conf)

    # use html5 backend if available (asciidoc >= 8.6.5)
    if(ASCIIDOC_VERSION VERSION_EQUAL "8.6.5" OR ASCIIDOC_VERSION VERSION_GREATER "8.6.5")
      set(ASCIIDOC_ARGS -b html5 ${ASCIIDOC_ARGS})
    endif()

    set(ASCIIDOC_ARGS_USER       ${ASCIIDOC_ARGS} ${CSS_TOC2}   )
    set(ASCIIDOC_ARGS_API        ${ASCIIDOC_ARGS} ${CSS_TOC2}   )
    set(ASCIIDOC_ARGS_SCRIPTING  ${ASCIIDOC_ARGS} ${CSS_TOC2}   )
    set(ASCIIDOC_ARGS_FAQ        ${ASCIIDOC_ARGS} ${CSS_TOC}  -n)
    set(ASCIIDOC_ARGS_QUICKSTART ${ASCIIDOC_ARGS} ${CSS_TOC2} -n)
    set(ASCIIDOC_ARGS_TESTER     ${ASCIIDOC_ARGS} ${CSS_TOC2}   )
    set(ASCIIDOC_ARGS_RELAY      ${ASCIIDOC_ARGS} ${CSS_TOC2}   )
    set(ASCIIDOC_ARGS_DEV        ${ASCIIDOC_ARGS} ${CSS_TOC2}   )

    add_subdirectory(en)
    add_subdirectory(fr)
    add_subdirectory(it)
    add_subdirectory(de)
    add_subdirectory(pl)
    add_subdirectory(es)
    add_subdirectory(ru)
    add_subdirectory(ja)

  endif()

endif()
