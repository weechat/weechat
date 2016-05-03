#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.it.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.it.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (it)"
  )
  add_custom_target(doc-man-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/it/man1)
endif()

if(ENABLE_DOC)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.it.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.it.html"
  )
  add_custom_target(doc-user-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.it.html"
  )
  add_custom_target(doc-plugin-api-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.it.html"
  )
  add_custom_target(doc-scripting-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.it.html"
  )
  add_custom_target(doc-faq-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.it.html"
  )
  add_custom_target(doc-quickstart-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_tester.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.it.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.it.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_tester.it.html"
  )
  add_custom_target(doc-tester-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
