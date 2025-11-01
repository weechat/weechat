#
# SPDX-FileCopyrightText: 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
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

if(CMAKE_VERSION VERSION_LESS "3.18.0")
  find_package(Python 3.0 REQUIRED COMPONENTS Development)
else()
  find_package(Python 3.0 REQUIRED COMPONENTS Development.Embed)
endif()

add_library(python MODULE
  weechat-python.c weechat-python.h
  weechat-python-api.c weechat-python-api.h
)
set_target_properties(python PROPERTIES PREFIX "")

include_directories(${Python_INCLUDE_DIRS})
add_definitions(${Python_DEFINITIONS})
target_link_libraries(python ${Python_LIBRARIES} weechat_plugins_scripts coverage_config)

install(TARGETS python LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
