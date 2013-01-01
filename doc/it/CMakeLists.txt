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
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.it.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
  COMMENT "Building weechat_user.it.html"
)
ADD_CUSTOM_TARGET(doc-user-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# plugin API reference
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_plugin_api.it.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/autogen/plugin_api/*.txt
  COMMENT "Building weechat_plugin_api.it.html"
)
ADD_CUSTOM_TARGET(doc-plugin-api-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# scripting guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.it.txt
  COMMENT "Building weechat_scripting.it.html"
)
ADD_CUSTOM_TARGET(doc-scripting-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.it.txt
  COMMENT "Building weechat_faq.it.html"
)
ADD_CUSTOM_TARGET(doc-faq-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.it.txt
  COMMENT "Building weechat_quickstart.it.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# tester's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a lang=it -a toc -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.it.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.it.txt
  COMMENT "Building weechat_tester.it.html"
)
ADD_CUSTOM_TARGET(doc-tester-it ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.it.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
