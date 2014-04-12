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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L ${CMAKE_CURRENT_BINARY_DIR}/weechat.1.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.txt
    COMMENT "Building weechat.1 (fr)"
  )
  add_custom_target(doc-man-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/fr/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
    COMMENT "Building weechat_user.fr.html"
  )
  add_custom_target(doc-user-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.txt
    COMMENT "Building weechat_plugin_api.fr.html"
  )
  add_custom_target(doc-plugin-api-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.txt
    COMMENT "Building weechat_scripting.fr.html"
  )
  add_custom_target(doc-scripting-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.txt
    COMMENT "Building weechat_faq.fr.html"
  )
  add_custom_target(doc-faq-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.txt
    COMMENT "Building weechat_quickstart.fr.html"
  )
  add_custom_target(doc-quickstart-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.txt
    COMMENT "Building weechat_tester.fr.html"
  )
  add_custom_target(doc-tester-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.fr.txt
    COMMENT "Building weechat_relay_protocol.fr.html"
  )
  add_custom_target(doc-relay-protocol-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.fr.txt
    COMMENT "Building weechat_dev.fr.html"
  )
  add_custom_target(doc-dev-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
