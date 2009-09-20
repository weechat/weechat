# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.de.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.de.txt
  COMMENT "Building weechat_faq.de.html"
)
ADD_CUSTOM_TARGET(doc-faq-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.de.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.de.txt
  COMMENT "Building weechat_quickstart.de.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# tester's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.de.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.de.txt
  COMMENT "Building weechat_tester.de.html"
)
ADD_CUSTOM_TARGET(doc-tester-de ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.de.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
