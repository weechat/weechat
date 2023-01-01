#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

if(ENABLE_MAN)
  # man page: weechat
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat.1
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.pl.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat.1.pl.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.pl.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.pl.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat.1 (pl)"
  )
  add_custom_target(doc-man-weechat-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat.1)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.1 DESTINATION ${MANDIR}/pl/man1)

  if(ENABLE_HEADLESS)
    # man page: weechat-headless
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1
      COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o weechat-headless.1 ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.pl.adoc
      DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/weechat-headless.1.pl.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.pl.adoc
      ${CMAKE_CURRENT_SOURCE_DIR}/includes/man.pl.adoc
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building weechat-headless.1 (pl)"
    )
    add_custom_target(doc-man-weechat-headless-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.1 DESTINATION ${MANDIR}/pl/man1)
  endif()
endif()

if(ENABLE_DOC)

  file(GLOB AUTOGEN_USER "${CMAKE_CURRENT_SOURCE_DIR}/includes/autogen_user_*.adoc")

  # user's guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_user.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.pl.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_user.pl.adoc
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/cmdline_options.pl.adoc
    ${AUTOGEN_USER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_user.pl.html"
  )
  add_custom_target(doc-user-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_user.pl.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # scripting guide
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_scripting.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_scripting.pl.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_scripting.pl.html"
  )
  add_custom_target(doc-scripting-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.pl.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # FAQ
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_faq.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_faq.pl.html"
  )
  add_custom_target(doc-faq-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

  # quickstart
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} ARGS ${ASCIIDOCTOR_ARGS} -o weechat_quickstart.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.adoc
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../docinfo.html
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.adoc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building weechat_quickstart.pl.html"
  )
  add_custom_target(doc-quickstart-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html DESTINATION ${DATAROOTDIR}/doc/${PROJECT_NAME})

endif()
