#!/bin/sh
rm -f config.cache

### GETTEXT ###

echo searching for GNU gettext intl directory...

dirs="/usr/share /usr/local/share /opt/share /usr /usr/local /opt /usr/gnu/share"
found=0
for try in $dirs; do
	echo -n " -> $try/gettext/intl... "
	if test -d $try/gettext/intl; then
		echo found it
		found=1
		break
	fi
	echo no
done
if test "$found" != 1; then
	echo ERROR: Cannot find gettext/intl directory.
	echo ERROR: Install GNU gettext in /usr or /usr/local prefix.
	exit 7
fi;

echo copying gettext intl files...
intldir="$try/gettext/intl"
if test ! -d intl; then
	mkdir intl
fi
olddir=`pwd`
cd $intldir
for file in *; do
	if test $file != COPYING.LIB-2.0 && test $file != COPYING.LIB-2.1; then
		rm -f $olddir/intl/$file
		cp $intldir/$file $olddir/intl/
	fi
done
cp -f $try/gettext/po/Makefile.in.in $olddir/po/
cd $olddir
if test -f intl/plural.c; then
	sleep 2
	touch intl/plural.c
fi

### END GETTEXT ###

echo "running aclocal..."
aclocal -I /usr/share/aclocal
echo "running autoconf..."
autoconf
echo "running autoheader..."
autoheader
echo "running automake..."
automake -a
echo "autogen.sh ok, now run ./configure script"
