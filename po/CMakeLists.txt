#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

# Check for programs xgettext, msgmerge and msgfmt
find_program(XGETTEXT_EXECUTABLE xgettext REQUIRED)
find_program(MSGMERGE_EXECUTABLE msgmerge REQUIRED)
find_program(MSGFMT_EXECUTABLE msgfmt REQUIRED)

set(PO_FILES
  cs.po
  de.po
  es.po
  fr.po
  hu.po
  it.po
  ja.po
  pl.po
  pt.po
  pt_BR.po
  ru.po
  sr.po
  tr.po
)

set(BUGS_ADDRESS "flashcode@flashtux.org")
set(POT_FILE "${PROJECT_NAME}.pot")
set(POT_FILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${POT_FILE}")

include(srcfiles.cmake)

# Create PO template file weechat.pot
set(SRC_FILES)
set(POT_DEPENDS)

foreach(srcfile ${WEECHAT_SOURCES})
  set(SRC_FILES ${SRC_FILES} ${srcfile})
  set(POT_DEPENDS ${POT_DEPENDS} "${CMAKE_SOURCE_DIR}/${srcfile}")
endforeach()

add_custom_command(
  OUTPUT "${POT_FILE_PATH}"
  COMMAND "${XGETTEXT_EXECUTABLE}"
  ARGS -o "${POT_FILE_PATH}" --add-comments='TRANSLATORS:' --keyword='_' --keyword='weechat_gettext' --keyword='N_' --keyword='NG_:1,2' --keyword='weechat_ngettext:1,2' --no-location --from-code=UTF-8 --directory="${CMAKE_SOURCE_DIR}" --package-name='WeeChat' --package-version=${VERSION} --msgid-bugs-address=${BUGS_ADDRESS} --copyright-holder='NAME' ${SRC_FILES}
  DEPENDS ${POT_DEPENDS}
  COMMENT "Generating ${POT_FILE}"
)

set(MO_FILES)
set(UPDATE_PO_TARGETS)
foreach(pofile ${PO_FILES})
  get_filename_component(polang ${pofile} NAME_WE)

  # Compile .po files in build directory (to binary .mo files)
  set(modir "${CMAKE_CURRENT_BINARY_DIR}/${polang}/LC_MESSAGES")
  file(MAKE_DIRECTORY "${modir}")
  set(mofile "${modir}/${PROJECT_NAME}.mo")
  add_custom_command(
    OUTPUT "${mofile}"
    COMMAND "${MSGMERGE_EXECUTABLE}" ARGS --quiet -o "${CMAKE_CURRENT_BINARY_DIR}/${pofile}" "${CMAKE_CURRENT_SOURCE_DIR}/${pofile}" ${POT_FILE_PATH}
    COMMAND "${MSGFMT_EXECUTABLE}" ARGS -o "${mofile}" "${CMAKE_CURRENT_BINARY_DIR}/${pofile}"
    COMMAND "${MSGFMT_EXECUTABLE}" ARGS -c --statistics --verbose --output-file=/dev/null "${CMAKE_CURRENT_BINARY_DIR}/${pofile}"
    DEPENDS "${POT_FILE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/${pofile}"
    COMMENT "Compiling ${polang}.po"
  )

  # Update .po files in source directory (if needed)
  add_custom_target(
    update-${pofile}
    COMMAND "${MSGMERGE_EXECUTABLE}" --quiet --update --previous --backup=none "${CMAKE_CURRENT_SOURCE_DIR}/${pofile}" "${POT_FILE_PATH}"
    COMMENT "Updating ${polang}.po"
  )
  set(UPDATE_PO_TARGETS ${UPDATE_PO_TARGETS} update-${pofile})

  install(FILES "${mofile}" DESTINATION "${LOCALEDIR}/${polang}/LC_MESSAGES")
  set(MO_FILES ${MO_FILES} ${mofile})
endforeach()

add_custom_target(translations ALL DEPENDS ${MO_FILES})

# Update weechat.pot in source directory (if needed)
add_custom_target(
  update-${POT_FILE}
  COMMAND "${MSGMERGE_EXECUTABLE}" --quiet --update --backup=none "${CMAKE_CURRENT_SOURCE_DIR}/${POT_FILE}" "${POT_FILE_PATH}"
  COMMENT "Updating ${POT_FILE}"
)

# Update all .po and weechat.pot in source directory (if needed)
add_custom_target(update-po DEPENDS update-${POT_FILE} ${UPDATE_PO_TARGETS})
