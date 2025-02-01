#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

  # docs languages
  set(AUTOGEN_LANG de en fr it ja pl sr)
  set(MAN_LANG cs de en fr it ja pl ru sr)
  set(USER_LANG de en fr it ja pl sr)
  set(PLUGIN_API_LANG en fr it ja sr)
  set(SCRIPTING_LANG de en fr it ja pl sr)
  set(FAQ_LANG de en es fr it ja pl sr)
  set(QUICKSTART_LANG cs de en es fr it ja pl ru sr)
  set(RELAY_API_LANG en fr)
  set(RELAY_WEECHAT_LANG en fr ja sr)
  set(DEV_LANG en fr ja sr)

  find_package(Asciidoctor)
  if(ASCIIDOCTOR_FOUND)
    # common asciidoctor arguments
    set(ASCIIDOCTOR_ARGS
      -a experimental
      -a reproducible
      -a "prewrap!"
      -a "webfonts!"
      -a revnumber="${VERSION}"
      -a sectanchors
      -a source-highlighter=pygments
      -a pygments-style=native
      -a docinfodir="${CMAKE_CURRENT_SOURCE_DIR}"
      -a autogendir="${CMAKE_CURRENT_BINARY_DIR}/autogen"
    )

    # asciidoctor arguments for user's guide
    set(ASCIIDOCTOR_USER_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for plugin API reference
    set(ASCIIDOCTOR_PLUGIN_API_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for scripting guide
    set(ASCIIDOCTOR_SCRIPTING_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for FAQ
    set(ASCIIDOCTOR_FAQ_ARGS
      -a toc=left
      -a toclevels=2
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for quickstart
    set(ASCIIDOCTOR_QUICKSTART_ARGS
      -a toc=left
      -a toclevels=2
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for relay "api"
    set(ASCIIDOCTOR_RELAY_API_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for relay "weechat"
    set(ASCIIDOCTOR_RELAY_WEECHAT_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    # asciidoctor arguments for developer's guide
    set(ASCIIDOCTOR_DEV_ARGS
      -a toc=left
      -a toclevels=3
      -a sectnums
      -a sectnumlevels=2
      -a docinfo1
    )

    if(ENABLE_DOC)
      set(AUTOGEN_FILES "")
      foreach(lang ${AUTOGEN_LANG})
        LIST(APPEND AUTOGEN_FILES
          # plugin API reference
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_completions.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_config_priority.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_hdata.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infolists.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infos.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infos_hashtable.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_plugins_priority.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_url_options.${lang}.adoc"
          # user's guide
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_commands.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_default_aliases.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_irc_colors.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_options.${lang}.adoc"
          # scripting duide
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_scripting_functions.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_scripting_constants.${lang}.adoc"
        )
      endforeach()
      add_custom_target(doc-autogen ALL
        COMMAND "${CMAKE_COMMAND}" -E env "WEECHAT_EXTRA_LIBDIR=${PROJECT_BINARY_DIR}/src" "WEECHAT_DOCGEN_LOCALEDIR=${PROJECT_BINARY_DIR}/po" "${CMAKE_BINARY_DIR}/src/gui/curses/headless/weechat-headless" --temp-dir --doc-gen "${CMAKE_CURRENT_BINARY_DIR}/autogen"
        DEPENDS
        # the headless binary is required
        weechat-headless
        # translations must be compiled
        translations
        # all plugins must be loaded during doc generation
        alias buflist charset exec fifo fset guile irc logger lua perl php python relay ruby script spell tcl trigger typing xfer
        BYPRODUCTS ${AUTOGEN_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      )
    endif()

    if(ENABLE_MAN)
      foreach(lang ${MAN_LANG})
        if(${lang} STREQUAL "en")
          set(MANDIR_LANG "${MANDIR}/man1")
        else()
          set(MANDIR_LANG "${MANDIR}/${lang}/man1")
        endif()
        # man page: weechat
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat.${lang}.1"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o "weechat.${lang}.1" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat.1.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat.1.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/cmdline_options.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/man.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat.1 (${lang})"
        )
        add_custom_target(doc-man-weechat-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat.${lang}.1")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat.${lang}.1" DESTINATION "${MANDIR_LANG}" RENAME "weechat.1")
        # man page: weechat-headless
        if(ENABLE_HEADLESS)
          add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.${lang}.1"
            COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} -b manpage -o "weechat-headless.${lang}.1" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat-headless.1.${lang}.adoc"
            DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat-headless.1.${lang}.adoc"
            "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/cmdline_options.${lang}.adoc"
            "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/man.${lang}.adoc"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Building weechat-headless.1 (${lang})"
          )
          add_custom_target(doc-man-weechat-headless-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.${lang}.1")
          install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat-headless.${lang}.1" DESTINATION "${MANDIR_LANG}" RENAME "weechat-headless.1")
        endif()
      endforeach()
    endif()

    if(ENABLE_DOC)
      # user's guide
      foreach(lang ${USER_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_user.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_USER_ARGS} -o "weechat_user.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_user.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_user.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/cmdline_options.${lang}.adoc"
          doc-autogen
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_commands.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_default_aliases.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_irc_colors.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_user_options.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_user.${lang}.html"
        )
        add_custom_target(doc-user-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_user.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_user.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # plugin API reference
      foreach(lang ${PLUGIN_API_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_PLUGIN_API_ARGS} -o "weechat_plugin_api.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_plugin_api.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_plugin_api.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          doc-autogen
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_completions.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_config_priority.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_hdata.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infolists.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infos.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_infos_hashtable.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_plugins_priority.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_api_url_options.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_plugin_api.${lang}.html"
        )
        add_custom_target(doc-plugin-api-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_plugin_api.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # scripting guide
      foreach(lang ${SCRIPTING_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_SCRIPTING_ARGS} -o "weechat_scripting.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_scripting.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_scripting.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          doc-autogen
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_scripting_functions.${lang}.adoc"
          "${CMAKE_CURRENT_BINARY_DIR}/autogen/autogen_scripting_constants.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_scripting.${lang}.html"
        )
        add_custom_target(doc-scripting-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_scripting.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # FAQ
      foreach(lang ${FAQ_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_FAQ_ARGS} -o "weechat_faq.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_faq.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_faq.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_faq.${lang}.html"
        )
        add_custom_target(doc-faq-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # quickstart
      foreach(lang ${QUICKSTART_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_QUICKSTART_ARGS} -o "weechat_quickstart.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_quickstart.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_quickstart.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_quickstart.${lang}.html"
        )
        add_custom_target(doc-quickstart-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # relay "api"
      foreach(lang ${RELAY_API_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_api.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_RELAY_API_ARGS} -o "weechat_relay_api.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_relay_api.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_relay_api.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/relay.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_relay_api.${lang}.html"
        )
        add_custom_target(doc-relay-api-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_api.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_api.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # relay "weechat"
      foreach(lang ${RELAY_WEECHAT_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_weechat.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_RELAY_WEECHAT_ARGS} -o "weechat_relay_weechat.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_relay_weechat.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_relay_weechat.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/relay.${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_relay_weechat.${lang}.html"
        )
        add_custom_target(doc-relay-weechat-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_weechat.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_relay_weechat.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
      # developer's guide
      foreach(lang ${DEV_LANG})
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.${lang}.html"
          COMMAND "${ASCIIDOCTOR_EXECUTABLE}" ARGS ${ASCIIDOCTOR_ARGS} ${ASCIIDOCTOR_DEV_ARGS} -o "weechat_dev.${lang}.html" "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_dev.${lang}.adoc"
          DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/docinfo.html"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/weechat_dev.${lang}.adoc"
          "${CMAKE_CURRENT_SOURCE_DIR}/${lang}/includes/attributes-${lang}.adoc"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMENT "Building weechat_dev.${lang}.html"
        )
        add_custom_target(doc-dev-${lang} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.${lang}.html")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat_dev.${lang}.html" DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}")
      endforeach()
    endif()

  else()
    message(SEND_ERROR "Asciidoctor not found")
  endif()

endif()
