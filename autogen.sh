#!/bin/sh

# gettextize updates Makefile.am, configure.in
cp configure.in configure.in.old
gettextize --copy --force --intl --no-changelog &&
mv configure.in.old configure.in
aclocal &&
# autoheader creates config.h.in needed by autoconf
autoheader &&
# autoconf creates configure
autoconf &&
# automake creates Makefile.in
automake --add-missing --copy --gnu
