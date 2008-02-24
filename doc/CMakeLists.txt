# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
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

SET(DOC_LANGS
  cs
  de
  en
  fr
  pl
  ru
  sco
  sv
  )

FIND_PROGRAM(
  COPY cp
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

FIND_PROGRAM(
  XSLTPROC xsltproc
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

FIND_PROGRAM(
  DBLATEX dblatex
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
  )

FIND_FILE(DOC_XSL_PREFIX chunk.xsl PATHS
  /usr/share/xml/docbook/stylesheet/nwalsh/
  /usr/share/xml/docbook/xsl-stylesheets-1.69/
  PATH_SUFFIXES html
  DOC "Path to nwalsh xsl stylesheet"
  )
STRING(REPLACE "/html/chunk.xsl" "" DOC_XSL_PREFIX "${DOC_XSL_PREFIX}")
      
IF(XSLTPROC AND DOC_XSL_PREFIX AND COPY)
  SET(BUILD_HTML TRUE)
ENDIF(XSLTPROC AND DOC_XSL_PREFIX AND COPY)

IF(DBLATEX AND COPY)
  SET(BUILD_PDF TRUE)
ENDIF(DBLATEX AND COPY)

IF(BUILD_HTML)
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html.xsl.in
    ${CMAKE_CURRENT_BINARY_DIR}/weechat-html.xsl
    @ONLY
    )

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
    ${CMAKE_CURRENT_BINARY_DIR}/weechat-html-one.xsl
    @ONLY
    )
ENDIF(BUILD_HTML)


IF(BUILD_HTML OR BUILD_PDF)
  EXECUTE_PROCESS(
    COMMAND date "+%F %T"
    OUTPUT_VARIABLE DOC_DATE
    )
  STRING(REPLACE "\n" "" DOC_DATE "${DOC_DATE}")

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/date.xml
    @ONLY
    )
  
  FOREACH(dlang ${DOC_LANGS})
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml)
      
      IF(BUILD_HTML)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang})
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang})
      
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/index.html ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/weechat.${dlang}.html
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/intro.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/install.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/usage.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/plugins.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/plugin_api.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/plugin_charset.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/plugin_scripts.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/authors.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/config.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/irc_commands.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/key_functions.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_commands.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}"
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/ ${CMAKE_CURRENT_BINARY_DIR}/weechat-html.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build/weechat.${dlang}.xml
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/weechat.${dlang}.html ${CMAKE_CURRENT_BINARY_DIR}/weechat-html-one.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}-build/weechat.${dlang}.xml
	  DEPENDS
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/config.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/irc_commands.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/key_functions.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_commands.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css
	  COMMENT "Building html doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-html-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/index.html)
	ADD_CUSTOM_TARGET(doc-html-one-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/weechat.${dlang}.html)
	INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang} DESTINATION share/doc/${PROJECT_NAME}/html)
	
      ENDIF(BUILD_HTML)
    
      IF(BUILD_PDF)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang})
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build)
	
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/weechat.${dlang}.pdf
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/config.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/irc_commands.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/key_functions.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_commands.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build"
	  COMMAND ${DBLATEX} ARGS -c ${CMAKE_CURRENT_SOURCE_DIR}/dblatex.conf -o ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/weechat.${dlang}.pdf ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}-build/weechat.${dlang}.xml
	  DEPENDS
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/config.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/irc_commands.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/key_functions.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_commands.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  COMMENT "Building pdf doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-pdf-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/weechat.${dlang}.pdf)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/weechat.${dlang}.pdf DESTINATION share/doc/${PROJECT_NAME})

      ENDIF(BUILD_PDF)
      
    ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat.${dlang}.xml)
  
  ENDFOREACH(dlang ${DOC_LANGS})

ENDIF(BUILD_HTML OR BUILD_PDF)

FOREACH(dlang ${DOC_LANGS})
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt)
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt DESTINATION share/doc/${PROJECT_NAME})
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt)
ENDFOREACH(dlang ${DOC_LANGS})

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/weechat-curses.1 DESTINATION share/man/man1)
