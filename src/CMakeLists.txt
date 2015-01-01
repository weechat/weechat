#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007-2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2008-2009 Emmanuel Bouthenot <kolter@openics.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
  add_definitions(-DWEECHAT_VERSION="${VERSION}" -DWEECHAT_LICENSE="${LICENSE}")
else()
  add_definitions(-DWEECHAT_VERSION='"${VERSION}"' -DWEECHAT_LICENSE='"${LICENSE}"')
endif()

add_subdirectory(core)
list(APPEND STATIC_LIBS weechat_core)

add_subdirectory(plugins)

add_subdirectory(gui)
