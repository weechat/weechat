#
# Copyright (C) 2006-2017 Adam Saponara <as@php.net>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(PHP_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = php.la

php_la_SOURCES = weechat-php.c \
                 weechat-php.h \
                 weechat-php-api.c \
                 weechat-php-api.h \
                 weechat-php_arginfo.h \
                 weechat-php_legacy_arginfo.h
php_la_LDFLAGS = -module -no-undefined
php_la_LIBADD  = ../lib_weechat_plugins_scripts.la $(PHP_LFLAGS)

EXTRA_DIST = CMakeLists.txt
