#!/bin/sh

# gettextize updates Makefile.am, configure.in
cp configure.in configure.in.old
cp Makefile.am Makefile.am.old
gettextize --copy --force --intl --no-changelog &&
mv Makefile.am.old Makefile.am
mv configure.in.old configure.in
libtoolize --force &&
aclocal &&
# autoheader creates config.h.in needed by autoconf
autoheader &&
# autoconf creates configure
autoconf &&
# automake creates Makefile.in
automake --add-missing --copy --gnu
