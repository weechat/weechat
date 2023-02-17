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
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.sr.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.sr.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (sr)"
  )
  add_custom_target(doc-man-weechat-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/sr/man1)

  if(ENABLE_HEADLESS)
    # man page: weechat-headless
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat-headless.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.sr.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.sr.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.sr.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.sr.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building weechat-headless.1 (sr)"
    )
    add_custom_target(doc-man-weechat-headless-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1 DESTINATION ${MANDIR}/sr/man1)
  endif()
endif()

if(ENABLE_DOC)

  file(GLOB AUTOGEN_USER "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_user_*.adoc")
  file(GLOB AUTOGEN_PLUGIN "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_api_*.adoc")

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.sr.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.sr.adoc
    ${AUTOGEN_USER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.sr.html"
  )
  add_custom_target(doc-user-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.sr.adoc
    ${AUTOGEN_PLUGIN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.sr.html"
  )
  add_custom_target(doc-plugin-api-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.sr.html"
  )
  add_custom_target(doc-scripting-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.sr.html"
  )
  add_custom_target(doc-faq-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.sr.html"
  )
  add_custom_target(doc-quickstart-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_relay_protocol.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.sr.html"
  )
  add_custom_target(doc-relay-protocol-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.sr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_dev.sr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.sr.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.sr.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.sr.html"
  )
  add_custom_target(doc-dev-sr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.sr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.sr.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

endif()
