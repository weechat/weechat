#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

if(ENABLE_MAN OR ENABLE_DOC)

  find_package(Asciidoctor)

  if(ASCIIDOCTOR_FOUND)

    set(ASCIIDOCTOR_ARGS -a experimental -a revnumber="${VERSION}" -a sectanchors -a source-highlighter=highlight.js)

    add_subdirectory(en)
    add_subdirectory(fr)
    add_subdirectory(it)
    add_subdirectory(de)
    add_subdirectory(pl)
    add_subdirectory(es)
    add_subdirectory(ru)
    add_subdirectory(ja)
    add_subdirectory(cs)

  endif()

endif()
