#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

if(ENABLE_MAN OR ENABLE_DOC)
  find_package(Asciidoctor)
  if(ASCIIDOCTOR_FOUND)
    # common asciidoctor arguments
    set(ASCIIDOCTOR_ARGS -a experimental -a reproducible -a "prewrap!" -a "webfonts!" -a icons=font -a revnumber="${VERSION}" -a sectanchors -a source-highlighter=pygments -a pygments-style=native)

    # sed arguments used to replace links in ChangeLog and release notes
    set(SED_LINKS_ARGS
      -e "'s/issue #\\([0-9][0-9]*\\)/https:\\/\\/github.com\\/weechat\\/weechat\\/issues\\/\\1[issue #\\1^]/g'"
      -e "'s/bug #\\([0-9][0-9]*\\)/https:\\/\\/savannah.nongnu.org\\/bugs\\/?\\1[bug #\\1^]/g'"
      -e "'s/task #\\([0-9][0-9]*\\)/https:\\/\\/savannah.nongnu.org\\/task\\/?\\1[task #\\1^]/g'"
      -e "'s/patch #\\([0-9][0-9]*\\)/https:\\/\\/savannah.nongnu.org\\/patch\\/?\\1[patch #\\1^]/g'"
      -e "'s/debian #\\([0-9][0-9]*\\)/http:\\/\\/bugs.debian.org\\/cgi-bin\\/bugreport.cgi?bug=\\1[debian bug #\\1^]/g'"
      -e "'s/\\(CVE-[0-9][0-9]*-[0-9][0-9]*\\)/https:\\/\\/cve.mitre.org\\/cgi-bin\\/cvename.cgi?name=\\1[\\1^]/g'"
    )

    # ChangeLog
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog-links.adoc
      COMMAND sed ARGS ${SED_LINKS_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/../ChangeLog.adoc > ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog-links.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/../ChangeLog.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.html
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -a docinfodir="${CMAKE_CURRENT_SOURCE_DIR}" -o ChangeLog.html ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog-links.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html
      ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog-links.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building ChangeLog.html"
    )
    add_custom_target(changelog DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.html)

    # Release notes
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes-links.adoc
      COMMAND sed ARGS ${SED_LINKS_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/../ReleaseNotes.adoc > ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes-links.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/../ReleaseNotes.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes.html
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -a docinfodir="${CMAKE_CURRENT_SOURCE_DIR}" -o ReleaseNotes.html ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes-links.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html
      ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes-links.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building ReleaseNotes.html"
    )
    add_custom_target(rn DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ReleaseNotes.html)

    # man/doc in all languages
    add_subdirectory(cs)
    add_subdirectory(de)
    add_subdirectory(en)
    add_subdirectory(es)
    add_subdirectory(fr)
    add_subdirectory(it)
    add_subdirectory(ja)
    add_subdirectory(pl)
    add_subdirectory(ru)
    add_subdirectory(sr)
  else()
    message(SEND_ERROR "Asciidoctor not found")
  endif()
endif()
