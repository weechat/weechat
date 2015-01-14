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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.asciidoc .
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.asciidoc .
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L weechat.1.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (ja)"
  )
  add_custom_target(doc-man-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/ja/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_USER} -o weechat_user.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.ja.html"
  )
  add_custom_target(doc-user-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_API} -o weechat_plugin_api.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.ja.html"
  )
  add_custom_target(doc-plugin-api-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_SCRIPTING} -o weechat_scripting.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.ja.html"
  )
  add_custom_target(doc-scripting-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_FAQ} -o weechat_faq.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.ja.html"
  )
  add_custom_target(doc-faq-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o weechat_quickstart.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.ja.html"
  )
  add_custom_target(doc-quickstart-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_TESTER} -o weechat_tester.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_tester.ja.html"
  )
  add_custom_target(doc-tester-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_RELAY} -o weechat_relay_protocol.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.ja.html"
  )
  add_custom_target(doc-relay-protocol-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_DEV} -o weechat_dev.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.ja.html"
  )
  add_custom_target(doc-dev-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
