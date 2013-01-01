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

SET(PO_FILES
  fr.po
  es.po
  cs.po
  hu.po
  de.po
  ru.po
  pl.po
  it.po
  ja.po
  pt_BR.po
  )

SET(BUGS_ADDRESS "flashcode@flashtux.org")
SET(POT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pot)

INCLUDE(srcfiles.cmake)

# Looking for xgettext, msgmerge and msgfmt
FIND_PROGRAM(XGETTEXT_EXECUTABLE xgettext)
FIND_PROGRAM(MSGMERGE_EXECUTABLE msgmerge)
FIND_PROGRAM(MSGFMT_EXECUTABLE msgfmt)

IF(XGETTEXT_EXECUTABLE AND MSGMERGE_EXECUTABLE AND MSGFMT_EXECUTABLE)

  # Create PO template file weechat.pot
  SET(SRC_FILES)
  SET(POT_DEPENDS)

  FOREACH(srcfile ${WEECHAT_SOURCES})
    SET(SRC_FILES ${SRC_FILES} ${srcfile})
    SET(POT_DEPENDS ${POT_DEPENDS} ${CMAKE_SOURCE_DIR}/${srcfile})
  ENDFOREACH(srcfile ${WEECHAT_SOURCES})

  ADD_CUSTOM_COMMAND(
    OUTPUT ${POT_FILE}
    COMMAND ${XGETTEXT_EXECUTABLE}
    ARGS -o ${POT_FILE} --add-comments='TRANSLATORS:' --keyword='_' --keyword='weechat_gettext' --keyword='N_' --keyword='NG_:1,2' --keyword='weechat_ngettext:1,2' --no-location --directory=${CMAKE_SOURCE_DIR} --msgid-bugs-address=${BUGS_ADDRESS} --copyright-holder='' ${SRC_FILES}
    DEPENDS ${POT_DEPENDS}
    COMMENT "Generating PO template file"
    )

  # Update .po files and compile them to binary .gmo files
  SET(GMO_FILES)
  FOREACH(pofile ${PO_FILES})
    GET_FILENAME_COMPONENT(polang ${pofile} NAME_WE)

    SET(gmofile ${CMAKE_CURRENT_BINARY_DIR}/${polang}.gmo)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${gmofile}
      COMMAND ${MSGMERGE_EXECUTABLE} ARGS --quiet -o ${CMAKE_CURRENT_BINARY_DIR}/${pofile} ${CMAKE_CURRENT_SOURCE_DIR}/${pofile} ${POT_FILE}
      COMMAND ${MSGFMT_EXECUTABLE} ARGS -o ${gmofile} ${CMAKE_CURRENT_BINARY_DIR}/${pofile}
      COMMAND ${MSGFMT_EXECUTABLE} ARGS -c --statistics --output-file=/dev/null ${CMAKE_CURRENT_BINARY_DIR}/${pofile}
      DEPENDS ${POT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${pofile}
      COMMENT "Compiling locales (${polang})"
      )

    INSTALL(FILES ${gmofile} DESTINATION ${LOCALEDIR}/${polang}/LC_MESSAGES RENAME ${PROJECT_NAME}.mo)
    SET(GMO_FILES ${GMO_FILES} ${gmofile})
  ENDFOREACH(pofile ${PO_FILES})
  ADD_CUSTOM_TARGET(translations ALL DEPENDS ${GMO_FILES})

ENDIF(XGETTEXT_EXECUTABLE AND MSGMERGE_EXECUTABLE AND MSGFMT_EXECUTABLE)
