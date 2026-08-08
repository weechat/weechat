# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

foreach(size 16 32 64 128 256 512)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${size}x${size}/weechat.png" DESTINATION "${DATAROOTDIR}/icons/hicolor/${size}x${size}/apps")
endforeach()
