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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.en.asciidoc .
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.en.asciidoc .
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L weechat.1.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.en.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (en)"
  )
  add_custom_target(doc-man-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/man1)
endif()

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_USER} -o weechat_user.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.en.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.en.html"
  )
  add_custom_target(doc-user-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_API} -o weechat_plugin_api.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.en.html"
  )
  add_custom_target(doc-plugin-api-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_SCRIPTING} -o weechat_scripting.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.en.html"
  )
  add_custom_target(doc-scripting-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_FAQ} -o weechat_faq.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.en.html"
  )
  add_custom_target(doc-faq-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_QUICKSTART} -o weechat_quickstart.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.en.html"
  )
  add_custom_target(doc-quickstart-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_TESTER} -o weechat_tester.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_tester.en.html"
  )
  add_custom_target(doc-tester-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_RELAY} -o weechat_relay_protocol.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.en.html"
  )
  add_custom_target(doc-relay-protocol-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS ${ASCIIDOC_ARGS_DEV} -o weechat_dev.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc_toc2.css
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.en.html"
  )
  add_custom_target(doc-dev-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
