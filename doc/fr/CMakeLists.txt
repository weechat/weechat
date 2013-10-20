#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
    COMMAND ${A2X_EXECUTABLE} ARGS -a lang=fr -a revision='WeeChat ${VERSION}' -d manpage -f manpage -L -D ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.txt
    COMMENT "Building weechat.1 (fr)"
  )
  ADD_CUSTOM_TARGET(doc-man-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/fr/man1)
ENDIF (ENABLE_MAN)

IF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # user's guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdline_options.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
    COMMENT "Building weechat_user.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-user-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # plugin API reference
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.fr.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.txt
    COMMENT "Building weechat_plugin_api.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-plugin-api-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # scripting guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.fr.txt
    COMMENT "Building weechat_scripting.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-scripting-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # FAQ
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.fr.txt
    COMMENT "Building weechat_faq.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-faq-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # quickstart
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.fr.txt
    COMMENT "Building weechat_quickstart.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-quickstart-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

  # tester's guide
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=fr -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.fr.txt
    COMMENT "Building weechat_tester.fr.html"
  )
  ADD_CUSTOM_TARGET(doc-tester-fr ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.fr.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

ENDIF(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)
