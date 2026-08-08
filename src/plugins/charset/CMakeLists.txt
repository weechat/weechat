# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(ICONV_FOUND)
  add_library(charset MODULE
    charset.c charset.h
  )
  set_target_properties(charset PROPERTIES PREFIX "")

  include_directories(${ICONV_INCLUDE_PATH})
  if(ICONV_LIBRARY)
    target_link_libraries(charset ${ICONV_LIBRARY} coverage_config)
  else()
    target_link_libraries(charset coverage_config)
  endif()

  install(TARGETS charset LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
else()
  message(SEND_ERROR "Iconv not found")
endif()
