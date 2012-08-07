#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.ja.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/autogen/user/*.txt
  COMMENT "Building weechat_user.ja.html"
)
ADD_CUSTOM_TARGET(doc-user-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc-title='目次' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.ja.txt
  COMMENT "Building weechat_faq.ja.html"
)
ADD_CUSTOM_TARGET(doc-faq-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc-title='目次' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.ja.txt
  COMMENT "Building weechat_quickstart.ja.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-ja ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.ja.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
