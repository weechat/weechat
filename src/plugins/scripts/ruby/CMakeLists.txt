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

ADD_LIBRARY(ruby MODULE ruby.c)

SET_TARGET_PROPERTIES(ruby PROPERTIES PREFIX "")

IF(RUBY_FOUND)
  INCLUDE_DIRECTORIES(${RUBY_INCLUDE_PATH})
  TARGET_LINK_LIBRARIES(ruby ${RUBY_LIBRARY} weechat_scripts)
ENDIF(RUBY_FOUND)

INSTALL(TARGETS ruby LIBRARY DESTINATION lib/${PROJECT_NAME}/plugins)
