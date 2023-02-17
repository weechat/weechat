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
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.en.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.en.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (en)"
  )
  add_custom_target(doc-man-weechat-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/man1)

  if(ENABLE_HEADLESS)
    # man page: weechat-headless
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat-headless.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.en.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.en.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.en.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.en.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building weechat-headless.1 (en)"
    )
    add_custom_target(doc-man-weechat-headless-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1 DESTINATION ${MANDIR}/man1)
  endif()
endif()

if(ENABLE_DOC)

  file(GLOB AUTOGEN_USER "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_user_*.adoc")
  file(GLOB AUTOGEN_PLUGIN "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_api_*.adoc")

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.en.adoc
    ${AUTOGEN_USER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.en.html"
  )
  add_custom_target(doc-user-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.adoc
    ${AUTOGEN_PLUGIN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.en.html"
  )
  add_custom_target(doc-plugin-api-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.en.html"
  )
  add_custom_target(doc-scripting-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.en.html"
  )
  add_custom_target(doc-faq-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.en.html"
  )
  add_custom_target(doc-quickstart-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_relay_protocol.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.en.html"
  )
  add_custom_target(doc-relay-protocol-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_dev.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.en.html"
  )
  add_custom_target(doc-dev-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

endif()
