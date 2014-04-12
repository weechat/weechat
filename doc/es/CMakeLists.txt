#
# Copyright (C) 2012-2014 Sébastien Helleu <flashcode@flashtux.org>
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

if(ENABLE_DOC AND SOURCEHIGHLIGHT_FOUND)

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.es.html
    COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a revision="${VERSION}" -a stylesheet=${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.css -f ${CMAKE_CURRENT_SOURCE_DIR}/../asciidoc.conf -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.es.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.es.txt
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.es.txt
    COMMENT "Building weechat_quickstart.es.html"
  )
  add_custom_target(doc-quickstart-es ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.es.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.es.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

endif()
