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
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (fr)"
  )
  add_custom_target(doc-man-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/fr/man1)
endif()

if(ENABLE_DOC)

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.fr.html"
  )
  add_custom_target(doc-user-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_plugin_api.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_plugin_api.fr.html"
  )
  add_custom_target(doc-plugin-api-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.fr.html"
  )
  add_custom_target(doc-scripting-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.fr.html"
  )
  add_custom_target(doc-faq-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.fr.html"
  )
  add_custom_target(doc-quickstart-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_tester.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_tester.fr.html"
  )
  add_custom_target(doc-tester-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # relay protocol
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_relay_protocol.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_relay_protocol.fr.html"
  )
  add_custom_target(doc-relay-protocol-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # developer's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_dev.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.fr.asciidoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.fr.asciidoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_dev.fr.html"
  )
  add_custom_target(doc-dev-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
