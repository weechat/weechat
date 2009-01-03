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
    
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/weechat_user.${dlang}.xml)
      
      IF(BUILD_HTML)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/autogen)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/autogen)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user)
        
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user/index.html ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user/weechat_user.${dlang}.html
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/autogen/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user/"
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user/ ${CMAKE_CURRENT_BINARY_DIR}/weechat-html.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/weechat_user.${dlang}.xml
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user/weechat_user.${dlang}.html ${CMAKE_CURRENT_BINARY_DIR}/weechat-html-one.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user-build/weechat_user.${dlang}.xml
	  DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css
	  COMMENT "Building user guide - html doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-user-html-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user/index.html)
	ADD_CUSTOM_TARGET(doc-user-html-one-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/user/weechat_user.${dlang}.html)
	INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/user DESTINATION share/doc/${PROJECT_NAME}/html/${dlang}/)
	
      ENDIF(BUILD_HTML)
      
      IF(BUILD_PDF)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/autogen)
        FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/autogen)
	
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user/weechat_user.${dlang}.pdf
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/autogen/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/"
	  COMMAND ${DBLATEX} ARGS -P draft.mode=no -c ${CMAKE_CURRENT_SOURCE_DIR}/dblatex.conf -o ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user/weechat_user.${dlang}.pdf ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user-build/weechat_user.${dlang}.xml
	  DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  COMMENT "Building user guide - pdf doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-user-pdf-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user/weechat_user.${dlang}.pdf)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/user/weechat_user.${dlang}.pdf DESTINATION share/doc/${PROJECT_NAME})

      ENDIF(BUILD_PDF)
      
    ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/user/weechat_user.${dlang}.xml)
    
    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/weechat_dev.${dlang}.xml)
      
      IF(BUILD_HTML)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/autogen)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/autogen)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev)
        
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev/index.html ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev/weechat_dev.${dlang}.html
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/autogen/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css" "${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev/"
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev/ ${CMAKE_CURRENT_BINARY_DIR}/weechat-html.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/weechat_dev.${dlang}.xml
	  COMMAND ${XSLTPROC} ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev/weechat_dev.${dlang}.html ${CMAKE_CURRENT_BINARY_DIR}/weechat-html-one.xsl ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev-build/weechat_dev.${dlang}.xml
	  DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html.xsl.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-doc.css
	  COMMENT "Building developer guide - html doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-dev-html-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev/index.html)
	ADD_CUSTOM_TARGET(doc-dev-html-one-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html-one/${dlang}/dev/weechat_dev.${dlang}.html)
	INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/${dlang}/dev DESTINATION share/doc/${PROJECT_NAME}/html/${dlang}/)
	
      ENDIF(BUILD_HTML)
      
      IF(BUILD_PDF)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev)
	FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev)
        FILE(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/autogen)
        FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/autogen)
        
	ADD_CUSTOM_COMMAND(
	  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev/weechat_dev.${dlang}.pdf
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/*.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/"
	  COMMAND ${COPY} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/autogen/"
          COMMAND ${COPY} ARGS "${CMAKE_CURRENT_BINARY_DIR}/date.xml" "${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/"
	  COMMAND ${DBLATEX} ARGS -P draft.mode=no -c ${CMAKE_CURRENT_SOURCE_DIR}/dblatex.conf -o ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev/weechat_dev.${dlang}.pdf ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev-build/weechat_dev.${dlang}.xml
	  DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/include_autogen.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/*.xml
	  ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/autogen/*
	  ${CMAKE_CURRENT_SOURCE_DIR}/date.xml.in
	  ${CMAKE_CURRENT_SOURCE_DIR}/weechat-html-one.xsl.in
	  COMMENT "Building developer guide - pdf doc (${dlang})"
	  )
	ADD_CUSTOM_TARGET(doc-dev-pdf-${dlang} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev/weechat_dev.${dlang}.pdf)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/pdf/${dlang}/dev/weechat_dev.${dlang}.pdf DESTINATION share/doc/${PROJECT_NAME})
        
      ENDIF(BUILD_PDF)
      
    ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/dev/weechat_dev.${dlang}.xml)
  
  ENDFOREACH(dlang ${DOC_LANGS})

ENDIF(BUILD_HTML OR BUILD_PDF)

FOREACH(dlang ${DOC_LANGS})
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt)
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt DESTINATION share/doc/${PROJECT_NAME})
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dlang}/weechat_quickstart.${dlang}.txt)
ENDFOREACH(dlang ${DOC_LANGS})

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/weechat-curses.1 DESTINATION share/man/man1)
