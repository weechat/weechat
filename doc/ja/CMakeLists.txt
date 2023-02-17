#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

if(ENABLE_MAN)
  # man page: weechat
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat.1
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.ja.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (ja)"
  )
  add_custom_target(doc-man-weechat-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/ja/man1)

  if(ENABLE_HEADLESS)
    # man page: weechat-headless
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat-headless.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.ja.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.ja.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.ja.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.ja.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building weechat-headless.1 (ja)"
    )
    add_custom_target(doc-man-weechat-headless-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1 DESTINATION ${MANDIR}/ja/man1)
  endif()
endif()

if(ENABLE_DOC)

  file(GLOB AUTOGEN_USER "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_user_*.adoc")
  file(GLOB AUTOGEN_PLUGIN "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_api_*.adoc")

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.ja.adoc
    ${AUTOGEN_USER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.ja.html"
  )
  add_custom_target(doc-user-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.adoc
    ${AUTOGEN_PLUGIN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.ja.html"
  )
  add_custom_target(doc-plugin-api-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.ja.html"
  )
  add_custom_target(doc-scripting-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.ja.html"
  )
  add_custom_target(doc-faq-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.ja.html"
  )
  add_custom_target(doc-quickstart-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_relay_protocol.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.ja.html"
  )
  add_custom_target(doc-relay-protocol-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_dev.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.ja.html"
  )
  add_custom_target(doc-dev-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

endif()
