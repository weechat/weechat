# Copyright (c) 2004 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

if PLUGIN_PERL
perl_dir = perl
endif

# if PLUGIN_PYTHON
# python_dir = python
# endif

# if PLUGIN_RUBY
# ruby_dir = ruby
# endif

# SUBDIRS = $(perl_dir) $(python_dir) $(ruby_dir)
SUBDIRS = $(perl_dir)

noinst_LIBRARIES = lib_weechat_plugins.a

lib_weechat_plugins_a_SOURCES = plugins.h \
                                plugins.c
