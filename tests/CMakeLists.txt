# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(ENABLE_FUZZ)
  add_subdirectory(fuzz)
endif()

if(ENABLE_TESTS)
  add_subdirectory(unit)
else()
  add_test(NAME notests COMMAND true)
endif()
