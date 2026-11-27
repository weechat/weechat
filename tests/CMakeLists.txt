# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(BUILD_FUZZERS)
  add_subdirectory(fuzz)
endif()

if(BUILD_TESTS)
  add_subdirectory(unit)
else()
  add_test(NAME notests COMMAND true)
endif()
