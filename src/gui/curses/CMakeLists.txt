# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
# SPDX-FileCopyrightText: 2007-2008 Julien Louis <ptitlouis@sysif.net>
# SPDX-FileCopyrightText: 2009 Emmanuel Bouthenot <kolter@openics.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(ENABLE_NCURSES)
  subdirs(normal)
endif()

if(ENABLE_HEADLESS)
  subdirs(headless)
endif()
