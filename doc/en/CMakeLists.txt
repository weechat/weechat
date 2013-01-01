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

# user's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.en.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
  COMMENT "Building weechat_user.en.html"
)
ADD_CUSTOM_TARGET(doc-user-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# plugin API reference
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.en.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.txt
  COMMENT "Building weechat_plugin_api.en.html"
)
ADD_CUSTOM_TARGET(doc-plugin-api-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# scripting guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.en.txt
  COMMENT "Building weechat_scripting.en.html"
)
ADD_CUSTOM_TARGET(doc-scripting-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.en.txt
  COMMENT "Building weechat_faq.en.html"
)
ADD_CUSTOM_TARGET(doc-faq-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.en.txt
  COMMENT "Building weechat_quickstart.en.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# tester's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.en.txt
  COMMENT "Building weechat_tester.en.html"
)
ADD_CUSTOM_TARGET(doc-tester-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# developer's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_dev.en.txt
  COMMENT "Building weechat_dev.en.html"
)
ADD_CUSTOM_TARGET(doc-dev-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# relay protocol
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_relay_protocol.en.txt
  COMMENT "Building weechat_relay_protocol.en.html"
)
ADD_CUSTOM_TARGET(doc-relay-protocol-en ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_protocol.en.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
