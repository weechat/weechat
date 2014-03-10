#
# Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(exec MODULE
exec.c exec.h
exec-command.c exec-command.h
exec-completion.c exec-completion.h
exec-config.c exec-config.h)
SET_TARGET_PROPERTIES(exec PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(exec)

INSTALL(TARGETS exec LIBRARY DESTINATION ${LIBDIR}/plugins)
