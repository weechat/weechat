#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.pl.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L ${CMAKE_CURRENT_BINARY_DIR}/weechat.1.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.pl.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.txt
    COMMENT "Building weechat.1 (pl)"
  )
  add_custom_target(doc-man-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/pl/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.pl.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.pl.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
    COMMENT "Building weechat_user.pl.html"
  )
  add_custom_target(doc-user-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.txt
    COMMENT "Building weechat_scripting.pl.html"
  )
  add_custom_target(doc-scripting-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
    COMMENT "Building weechat_faq.pl.html"
  )
  add_custom_target(doc-faq-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
    COMMENT "Building weechat_quickstart.pl.html"
  )
  add_custom_target(doc-quickstart-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
    COMMENT "Building weechat_tester.pl.html"
  )
  add_custom_target(doc-tester-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
