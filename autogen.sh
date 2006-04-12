#!/bin/sh

# gettextize updates Makefile.am, configure.in
cp configure.in configure.in.old
cp Makefile.am Makefile.am.old
if test "$1" = "--auto" ; then
    grep -v 'read dummy < /dev/tty' $(which gettextize) | /bin/sh -s -- --copy --force --intl --no-changelog
else
    gettextize --copy --force --intl --no-changelog
fi
mv Makefile.am.old Makefile.am
mv configure.in.old configure.in
libtoolize --automake --force --copy
aclocal
# autoheader creates config.h.in needed by autoconf
autoheader
# autoconf creates configure
autoconf
# automake creates Makefile.in
automake --add-missing --copy --gnu
