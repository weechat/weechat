#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
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

if(ENABLE_MAN)
  # man page
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat.1
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.de.asciidoc ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L ${CMAKE_CURRENT_BINARY_DIR}/weechat.1.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc
    COMMENT "Building weechat.1 (de)"
  )
  add_custom_target(doc-man-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/de/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.de.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    COMMENT "Building weechat_user.de.html"
  )
  add_custom_target(doc-user-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.de.asciidoc
    COMMENT "Building weechat_scripting.de.html"
  )
  add_custom_target(doc-scripting-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.de.asciidoc
    COMMENT "Building weechat_faq.de.html"
  )
  add_custom_target(doc-faq-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.de.asciidoc
    COMMENT "Building weechat_quickstart.de.html"
  )
  add_custom_target(doc-quickstart-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.de.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.de.asciidoc
    COMMENT "Building weechat_tester.de.html"
  )
  add_custom_target(doc-tester-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
