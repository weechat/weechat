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

# scripting guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toclevels=3 -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.txt
  COMMENT "Building weechat_scripting.pl.html"
)
ADD_CUSTOM_TARGET(doc-scripting-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
  COMMENT "Building weechat_faq.pl.html"
)
ADD_CUSTOM_TARGET(doc-faq-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
  COMMENT "Building weechat_quickstart.pl.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# tester's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
  COMMENT "Building weechat_tester.pl.html"
)
ADD_CUSTOM_TARGET(doc-tester-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
