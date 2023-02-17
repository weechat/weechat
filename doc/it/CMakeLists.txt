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
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.it.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.it.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.it.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (it)"
  )
  add_custom_target(doc-man-weechat-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/it/man1)

  if(ENABLE_HEADLESS)
    # man page: weechat-headless
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat-headless.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.it.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.it.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.it.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.it.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building weechat-headless.1 (it)"
    )
    add_custom_target(doc-man-weechat-headless-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1 DESTINATION ${MANDIR}/it/man1)
  endif()
endif()

if(ENABLE_DOC)

  file(GLOB AUTOGEN_USER "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_user_*.adoc")
  file(GLOB AUTOGEN_PLUGIN "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_api_*.adoc")

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.it.adoc
    ${AUTOGEN_USER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.it.html"
  )
  add_custom_target(doc-user-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.adoc
    ${AUTOGEN_PLUGIN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.it.html"
  )
  add_custom_target(doc-plugin-api-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.it.html"
  )
  add_custom_target(doc-scripting-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.it.html"
  )
  add_custom_target(doc-faq-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.it.html"
  )
  add_custom_target(doc-quickstart-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

endif()
