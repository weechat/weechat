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

IF (ENABLE_MAN)
  # man page
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat.1
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.txt ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${A2X_EXECUTABLE} ARGS -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L ${CMAKE_CURRENT_BINARY_DIR}/weechat.1.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.ja.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.txt
    COMMENT "Building weechat.1 (ja)"
    )
  ADD_CUSTOM_TARGET(doc-man-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/ja/man1)
ENDIF (ENABLE_MAN)

IF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.ja.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
    COMMENT "Building weechat_user.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-user-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.ja.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.txt
    COMMENT "Building weechat_plugin_api.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-plugin-api-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.ja.txt
    COMMENT "Building weechat_scripting.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-scripting-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.txt
    COMMENT "Building weechat_faq.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-faq-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.txt
    COMMENT "Building weechat_quickstart.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-quickstart-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.ja.txt
    COMMENT "Building weechat_tester.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-tester-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # developer's guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.ja.txt
    COMMENT "Building weechat_dev.ja.html"
  )
  ADD_CUSTOM_TARGET(doc-dev-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

ENDIF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)
