#!/bin/sh

# gettextize updates Makefile.am, configure.in
gettextize --copy --force --intl --no-changelog &&
aclocal &&
# autoheader creates config.h.in needed by autoconf
autoheader &&
# autoconf creates configure
autoconf &&
# automake creates Makefile.in
automake --add-missing --copy --gnu
