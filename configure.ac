#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2005 Benoit Papillault <benoit.papillault@free.fr>
# Copyright (C) 2005-2006 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2005-2009 Emmanuel Bouthenot <kolter@openics.org>
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

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_INIT(WeeChat, m4_esyscmd([./version.sh devel-full | tr -d '\n']), flashcode@flashtux.org)
AC_CONFIG_SRCDIR([configure.ac])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign])
LICENSE="GPL3"

# Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_GNU_SOURCE
AC_PROG_LN_S
AM_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Files to generate
AC_CONFIG_FILES([weechat.pc])
AC_CONFIG_FILES([weechat-${VERSION}-1.cygport:weechat.cygport.in])

# Add some flags for some OS
case "$host_os" in
freebsd* | openbsd*)
        CFLAGS="$CFLAGS -I/usr/local/include"
        CXXFLAGS="$CXXFLAGS -I/usr/local/include"
        LDFLAGS="$LDFLAGS -L/usr/local/lib"
        ;;
netbsd*)
        CFLAGS="$CFLAGS -I/usr/pkg/include"
        CXXFLAGS="$CXXFLAGS -I/usr/pkg/include"
        LDFLAGS="$LDFLAGS -L/usr/pkg/lib"
        ;;
solaris*)
        LDFLAGS="$LDFLAGS -lsocket -lxnet"
        ;;
darwin*)
        LDFLAGS="$LDFLAGS -lresolv"
        ;;
esac

# Gettext
ALL_LINGUAS="cs de es fr hu it ja pl pt pt_BR ru sr tr"
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18])

# Checks for libraries
AC_CHECK_LIB(ncurses, initscr, LIBNCURSES_FOUND=1, LIBNCURSES_FOUND=0)
AC_CHECK_LIB(ncursesw, initscr, LIBNCURSESW_FOUND=1, LIBNCURSESW_FOUND=0)

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h sys/resource.h])

# Checks for typedefs, structures, and compiler characteristics
AC_HEADER_TIME
AC_STRUCT_TM

AC_MSG_CHECKING([for socklen_t])
AC_CACHE_VAL(ac_cv_type_socklen_t,
[AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[ #include <sys/types.h>
#include <sys/socket.h> ]],
[[ socklen_t t; ]])],
[ ac_cv_type_socklen_t=yes ],
[ ac_cv_type_socklen_t=no ],
)])
if test $ac_cv_type_socklen_t = no; then
AC_DEFINE(socklen_t, int, Define to 'int' if <sys/socket.h> doesn't define.)
fi
AC_MSG_RESULT($ac_cv_type_socklen_t)

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([mallinfo])
AC_CHECK_FUNCS([mallinfo2])

# Variables in config.h

AH_VERBATIM([PREFIX], [#undef PREFIX])
AH_VERBATIM([WEECHAT_LIBDIR], [#undef WEECHAT_LIBDIR])
AH_VERBATIM([WEECHAT_SHAREDIR], [#undef WEECHAT_SHAREDIR])
AH_VERBATIM([HAVE_FLOCK], [#undef HAVE_FLOCK])
AH_VERBATIM([HAVE_EAT_NEWLINE_GLITCH], [#undef HAVE_EAT_NEWLINE_GLITCH])
AH_VERBATIM([HAVE_ASPELL_VERSION_STRING], [#undef HAVE_ASPELL_VERSION_STRING])
AH_VERBATIM([HAVE_ENCHANT_GET_VERSION], [#undef HAVE_ENCHANT_GET_VERSION])
AH_VERBATIM([HAVE_GUILE_GMP_MEMORY_FUNCTIONS], [#undef HAVE_GUILE_GMP_MEMORY_FUNCTIONS])
AH_VERBATIM([PLUGIN_ALIAS], [#undef PLUGIN_ALIAS])
AH_VERBATIM([PLUGIN_BUFLIST], [#undef PLUGIN_BUFLIST])
AH_VERBATIM([PLUGIN_CHARSET], [#undef PLUGIN_CHARSET])
AH_VERBATIM([PLUGIN_EXEC], [#undef PLUGIN_EXEC])
AH_VERBATIM([PLUGIN_FIFO], [#undef PLUGIN_FIFO])
AH_VERBATIM([PLUGIN_FSET], [#undef PLUGIN_FSET])
AH_VERBATIM([PLUGIN_IRC], [#undef PLUGIN_IRC])
AH_VERBATIM([PLUGIN_LOGGER], [#undef PLUGIN_LOGGER])
AH_VERBATIM([PLUGIN_RELAY], [#undef PLUGIN_RELAY])
AH_VERBATIM([PLUGIN_SCRIPT], [#undef PLUGIN_SCRIPT])
AH_VERBATIM([PLUGIN_PERL], [#undef PLUGIN_PERL])
AH_VERBATIM([PLUGIN_PYTHON], [#undef PLUGIN_PYTHON])
AH_VERBATIM([PLUGIN_RUBY], [#undef PLUGIN_RUBY])
AH_VERBATIM([PLUGIN_LUA], [#undef PLUGIN_LUA])
AH_VERBATIM([PLUGIN_PHP], [#undef PLUGIN_PHP])
AH_VERBATIM([PLUGIN_TCL], [#undef PLUGIN_TCL])
AH_VERBATIM([PLUGIN_GUILE], [#undef PLUGIN_GUILE])
AH_VERBATIM([PLUGIN_JAVASCRIPT], [#undef PLUGIN_JAVASCRIPT])
AH_VERBATIM([PLUGIN_SPELL], [#undef PLUGIN_SPELL])
AH_VERBATIM([PLUGIN_TRIGGER], [#undef PLUGIN_TRIGGER])
AH_VERBATIM([PLUGIN_TYPING], [#undef PLUGIN_TYPING])
AH_VERBATIM([PLUGIN_XFER], [#undef PLUGIN_XFER])
AH_VERBATIM([TESTS], [#undef TESTS])
AH_VERBATIM([MAN], [#undef MAN])
AH_VERBATIM([DOC], [#undef DOC])
AH_VERBATIM([WEECHAT_HOME], [#define WEECHAT_HOME ""])

# Arguments for ./configure

AC_ARG_ENABLE(ncurses,      [  --disable-ncurses       turn off ncurses interface (default=compiled if found)],enable_ncurses=$enableval,enable_ncurses=yes)
AC_ARG_ENABLE(headless,     [  --disable-headless      turn off headless binary (default=compiled), this is required for tests],enable_headless=$enableval,enable_headless=yes)
AC_ARG_ENABLE(largefile,    [  --disable-largefile     turn off Large File Support (default=on)],enable_largefile=$enableval,enable_largefile=yes)
AC_ARG_ENABLE(alias,        [  --disable-alias         turn off Alias plugin (default=compiled)],enable_alias=$enableval,enable_alias=yes)
AC_ARG_ENABLE(buflist,      [  --disable-buflist       turn off Buflist plugin (default=compiled)],enable_buflist=$enableval,enable_buflist=yes)
AC_ARG_ENABLE(charset,      [  --disable-charset       turn off Charset plugin (default=compiled)],enable_charset=$enableval,enable_charset=yes)
AC_ARG_ENABLE(exec,         [  --disable-exec          turn off Exec plugin (default=compiled)],enable_exec=$enableval,enable_exec=yes)
AC_ARG_ENABLE(fifo,         [  --disable-fifo          turn off Fifo plugin (default=compiled)],enable_fifo=$enableval,enable_fifo=yes)
AC_ARG_ENABLE(fset,         [  --disable-fset          turn off Fast Set plugin (default=compiled)],enable_fset=$enableval,enable_fset=yes)
AC_ARG_ENABLE(irc,          [  --disable-irc           turn off IRC plugin (default=compiled)],enable_irc=$enableval,enable_irc=yes)
AC_ARG_ENABLE(logger,       [  --disable-logger        turn off Logger plugin (default=compiled)],enable_logger=$enableval,enable_logger=yes)
AC_ARG_ENABLE(relay,        [  --disable-relay         turn off Relay plugin (default=compiled)],enable_relay=$enableval,enable_relay=yes)
AC_ARG_ENABLE(script,       [  --disable-script        turn off Script plugin (default=compiled)],enable_script=$enableval,enable_script=yes)
AC_ARG_ENABLE(scripts,      [  --disable-scripts       turn off script plugins (perl, python, ...) (default=compiled if found)],enable_scripts=$enableval,enable_scripts=yes)
AC_ARG_ENABLE(perl,         [  --disable-perl          turn off Perl script plugin (default=compiled if found)],enable_perl=$enableval,enable_perl=yes)
AC_ARG_ENABLE(python,       [  --disable-python        turn off Python script plugin (default=compiled if found)],enable_python=$enableval,enable_python=yes)
AC_ARG_ENABLE(ruby,         [  --disable-ruby          turn off Ruby script plugin (default=compiled if found)],enable_ruby=$enableval,enable_ruby=yes)
AC_ARG_ENABLE(lua,          [  --disable-lua           turn off Lua script plugin (default=compiled if found)],enable_lua=$enableval,enable_lua=yes)
AC_ARG_ENABLE(tcl,          [  --disable-tcl           turn off Tcl script plugin (default=compiled if found)],enable_tcl=$enableval,enable_tcl=yes)
AC_ARG_ENABLE(guile,        [  --disable-guile         turn off Guile (scheme) script plugin (default=compiled if found)],enable_guile=$enableval,enable_guile=yes)
AC_ARG_ENABLE(javascript,   [  --enable-javascript     turn on JavaScript script plugin (default=off)],enable_javascript=$enableval,enable_javascript=no)
AC_ARG_ENABLE(php,          [  --disable-php           turn off PHP script plugin (default=compiled if found)],enable_php=$enableval,enable_php=yes)
AC_ARG_ENABLE(spell,        [  --disable-spell         turn off Spell checker plugin (default=compiled)],enable_spell=$enableval,enable_spell=yes)
AC_ARG_ENABLE(enchant,      [  --enable-enchant        turn on Enchant lib for Spell checker plugin (default=off)],enable_enchant=$enableval,enable_enchant=no)
AC_ARG_ENABLE(trigger,      [  --disable-trigger       turn off Trigger plugin (default=compiled)],enable_trigger=$enableval,enable_trigger=yes)
AC_ARG_ENABLE(typing,       [  --disable-typing        turn off Typing plugin (default=compiled)],enable_trigger=$enableval,enable_typing=yes)
AC_ARG_ENABLE(xfer,         [  --disable-xfer          turn off Xfer (file transfer) plugin (default=compiled)],enable_xfer=$enableval,enable_xfer=yes)
AC_ARG_WITH(tclconfig,      [  --with-tclconfig=DIR    directory containing tcl configuration (tclConfig.sh)],tclconfig=$withval,tclconfig='')
AC_ARG_WITH(debug,          [  --with-debug            debugging: 0=no debug, 1=debug compilation (default=0)],debug=$withval,debug=0)
AC_ARG_ENABLE(tests,        [  --enable-tests          turn on build of tests (default=not built)],enable_tests=$enableval,enable_tests=no)
AC_ARG_ENABLE(man,          [  --enable-man            turn on build of man page (default=not built)],enable_man=$enableval,enable_man=no)
AC_ARG_ENABLE(doc,          [  --enable-doc            turn on build of documentation (default=not built)],enable_doc=$enableval,enable_doc=no)

AC_ARG_VAR(WEECHAT_HOME, [Force a single WeeChat home directory for config, logs, scripts, etc.])

AC_DEFINE_UNQUOTED(WEECHAT_HOME, "$WEECHAT_HOME")

not_asked=""
not_found=""

if test "x$enable_headless" != "xyes" && test "x$enable_tests" = "xyes"; then
    AC_MSG_ERROR([*** Headless mode is required for tests.])
fi

# ------------------------------------------------------------------------------
#                                  pkg-config
# ------------------------------------------------------------------------------

PKGCONFIG=""
AC_CHECK_PROGS(PKGCONFIG, pkg-config)
if test "x$PKGCONFIG" = "x"; then
    AC_MSG_ERROR([
*** "pkg-config" couldn't be found on your system.
*** Try to install it with your software package manager.])
fi

# ------------------------------------------------------------------------------
#                                dynamic loader
# ------------------------------------------------------------------------------

PLUGINS_LFLAGS=

AC_CHECK_FUNCS(dlopen, LIBDL_FOUND=yes, LIBDL_FOUND=no)
if test "$LIBDL_FOUND" != "yes"; then
    AC_CHECK_LIB(dl, dlopen, [LIBDL_FOUND=yes; PLUGINS_LFLAGS=-ldl], LIBDL_FOUND=no)
fi
if test "$LIBDL_FOUND" != "yes"; then
    AC_MSG_ERROR([
*** "dl" library (dynamic library loader) couldn't be found on your system.
*** Try to install it with your software package manager.])
fi
AC_SUBST(PLUGINS_LFLAGS)

# ------------------------------------------------------------------------------
#                                    gui
# ------------------------------------------------------------------------------

if test "x$enable_ncurses" = "xyes" ; then
    if test "$LIBNCURSESW_FOUND" = "0" ; then
        if test "$LIBNCURSES_FOUND" = "0" ; then
            AC_MSG_WARN([
*** ncurses library not found!
*** WeeChat will be built without ncurses support.])
            enable_ncurses="no"
            not_found="$not_found ncurses"
        else
            AC_MSG_WARN([
*** ncursesw library not found! Falling back to "ncurses"
*** Be careful, UTF-8 display may not work properly if your locale is UTF-8.])
            NCURSES_LFLAGS="-lncurses"
        fi
    else
        NCURSES_LFLAGS="-lncursesw"
    fi
    AC_CHECK_HEADERS([ncurses.h ncursesw/curses.h])
    AC_SUBST(NCURSES_LFLAGS)
else
    not_asked="$not_asked ncurses"
fi

# ------------------------------------------------------------------------------
#                                   headless
# ------------------------------------------------------------------------------

if test "x$enable_headless" != "xyes" ; then
    not_asked="$not_asked headless"
fi

# ------------------------------------------------------------------------------
#                                    iconv
# ------------------------------------------------------------------------------

ICONV_LFLAGS=""

iconv_found="no"
AC_CHECK_HEADER(iconv.h,ac_found_iconv_header="yes",ac_found_iconv_header="no")
if test "x$ac_found_iconv_header" = "xyes" ; then
   AC_CHECK_LIB(iconv,iconv_open,ac_found_iconv_lib="yes",ac_found_iconv_lib="no")
   if test "x$ac_found_iconv_lib" = "xno" ; then
      AC_CHECK_LIB(iconv,libiconv_open,ac_found_iconv_lib="yes",ac_found_iconv_lib="no")
   fi
   if test "x$ac_found_iconv_lib" = "xyes" ; then
      ICONV_LFLAGS="-liconv"
      LIBS="$LIBS $ICONV_LFLAGS"
   fi
   AC_MSG_CHECKING(for iconv usability in programs)
   AC_TRY_RUN([
        #include <iconv.h>
        int main(int argc, char **argv) {
            iconv_t conv = iconv_open("ISO8859-1", "UTF-8");
            if (conv != (iconv_t) -1) {
                return 0;
            }
            return 1;
      }],iconv_found="yes", iconv_found="no", iconv_found="assume-yes")
  if test "x$iconv_found" = "xno" ; then
     AC_MSG_RESULT(no)
  else
     AC_MSG_RESULT(yes)
  fi
fi

if test "x$iconv_found" = "xno" ; then
   AC_MSG_ERROR([
*** Iconv headers and/or libraries couldn't be found on your system.
*** Try to install them with your software package manager.
*** WeeChat can't be built without Iconv support.])
fi

# ------------------------------------------------------------------------------
#                                   plugins
# ------------------------------------------------------------------------------

if test "x$enable_scripts" = "xno" ; then
    enable_perl="no"
    enable_python="no"
    enable_ruby="no"
    enable_lua="no"
    enable_tcl="no"
    enable_guile="no"
    enable_javascript="no"
    enable_php="no"
fi

# ---------------------------------- alias -------------------------------------

if test "x$enable_alias" = "xyes" ; then
    ALIAS_CFLAGS=""
    ALIAS_LFLAGS=""
    AC_SUBST(ALIAS_CFLAGS)
    AC_SUBST(ALIAS_LFLAGS)
    AC_DEFINE(PLUGIN_ALIAS)
else
    not_asked="$not_asked alias"
fi

# --------------------------------- buflist ------------------------------------

if test "x$enable_buflist" = "xyes" ; then
    AC_DEFINE(PLUGIN_BUFLIST)
else
    not_asked="$not_asked buflist"
fi

# --------------------------------- charset ------------------------------------

if test "x$enable_charset" = "xyes" ; then
    CHARSET_CFLAGS=""
    CHARSET_LFLAGS="$ICONV_LFLAGS"
    AC_SUBST(CHARSET_CFLAGS)
    AC_SUBST(CHARSET_LFLAGS)
    AC_DEFINE(PLUGIN_CHARSET)
else
    not_asked="$not_asked charset"
fi

# ---------------------------------- exec --------------------------------------

if test "x$enable_exec" = "xyes" ; then
    EXEC_CFLAGS=""
    EXEC_LFLAGS=""
    AC_SUBST(EXEC_CFLAGS)
    AC_SUBST(EXEC_LFLAGS)
    AC_DEFINE(PLUGIN_EXEC)
else
    not_asked="$not_asked exec"
fi

# ---------------------------------- fifo --------------------------------------

if test "x$enable_fifo" = "xyes" ; then
    FIFO_CFLAGS=""
    FIFO_LFLAGS=""
    AC_SUBST(FIFO_CFLAGS)
    AC_SUBST(FIFO_LFLAGS)
    AC_DEFINE(PLUGIN_FIFO)
else
    not_asked="$not_asked fifo"
fi

# --------------------------------- fset ---------------------------------------

if test "x$enable_fset" = "xyes" ; then
    AC_DEFINE(PLUGIN_FSET)
else
    not_asked="$not_asked fset"
fi

# ----------------------------------- irc --------------------------------------

if test "x$enable_irc" = "xyes" ; then
    AC_DEFINE(PLUGIN_IRC)
else
    not_asked="$not_asked irc"
fi

# --------------------------------- logger -------------------------------------

if test "x$enable_logger" = "xyes" ; then
    LOGGER_CFLAGS=""
    LOGGER_LFLAGS=""
    AC_SUBST(LOGGER_CFLAGS)
    AC_SUBST(LOGGER_LFLAGS)
    AC_DEFINE(PLUGIN_LOGGER)
else
    not_asked="$not_asked logger"
fi

# --------------------------------- relay --------------------------------------

if test "x$enable_relay" = "xyes" ; then
    RELAY_CFLAGS=""
    RELAY_LFLAGS=""
    AC_SUBST(RELAY_CFLAGS)
    AC_SUBST(RELAY_LFLAGS)
    AC_DEFINE(PLUGIN_RELAY)
else
    not_asked="$not_asked relay"
fi

# --------------------------------- script -------------------------------------

if test "x$enable_script" = "xyes" ; then
    AC_DEFINE(PLUGIN_SCRIPT)
else
    not_asked="$not_asked script"
fi

# ---------------------------------- perl --------------------------------------

PERL_VERSION=

if test "x$enable_perl" = "xyes" ; then
    AC_PATH_PROGS(PERL, perl perl5)
    if test -z $PERL ; then
        AC_MSG_WARN([
*** Perl must be installed on your system but perl interpreter couldn't be found in path.
*** Please check that perl is in path, or install it with your software package manager.
*** WeeChat will be built without Perl support.])
        enable_perl="no"
        not_found="$not_found perl"
    else
        PERL_VERSION=`perl -V:version | sed "s/version='\(.*\)';/\1/"`
        AC_MSG_CHECKING(for Perl headers files)

        PERL_HEADER_TEST=`PT=perltest.c ; echo "#include <EXTERN.h>" > $PT;  echo "#include <perl.h>" >> $PT;  echo "#include <XSUB.h>" >> $PT ;  echo "int main() { return 0; }" >> $PT ; $CC -Wall $PT -o $PT.out $($PERL -MExtUtils::Embed -e ccopts -e ldopts) 1>/dev/null 2>&1; echo $?; rm -f $PT $PT.out 1>/dev/null 2>&1`

        if test "x$PERL_HEADER_TEST" = "x0" ; then
            PERL_CFLAGS=`$PERL -MExtUtils::Embed -e ccopts`
            AC_MSG_RESULT(found)
            AC_MSG_CHECKING(for Perl library)
            PERL_LIB_TEST=`PT=perltest.c ; echo "int main() { return 0; }" > $PT ; $CC -Wall $PT -o $PT.out $($PERL -MExtUtils::Embed -e ldopts) 1>/dev/null 2>&1; echo $?; rm -f $PT $PT.out 1>/dev/null 2>&1`
            if test "x$PERL_LIB_TEST" = "x0" ; then
                PERL_LFLAGS=`$PERL -MExtUtils::Embed -e ldopts`
                AC_MSG_RESULT(found)
            else
                AC_MSG_WARN([
*** Perl library couldn't be found on your system.
*** Try to install it with your software package manager.
*** WeeChat will be built without Perl support.])
                enable_perl="no"
                not_found="$not_found perl"
            fi
        else
            AC_MSG_WARN([
*** Perl headers couldn't be found on your system.
*** Try to install it with your software package manager.
*** WeeChat will be built without Perl support.])
            enable_perl="no"
            not_found="$not_found perl"
        fi
    fi
else
    not_asked="$not_asked perl"
fi

if test "x$enable_perl" = "xyes" ; then
    AC_SUBST(PERL_CFLAGS)
    AC_SUBST(PERL_LFLAGS)
    AC_DEFINE(PLUGIN_PERL)
fi

# --------------------------------- python -------------------------------------

PYTHON_VERSION=

if test "x$enable_python" = "xyes" ; then
    PKG_CHECK_MODULES(PYTHON, [python3-embed], [PYTHON_FOUND=yes; PYTHON_VERSION=`$PKGCONFIG --modversion python3-embed`], [PYTHON_FOUND=no])
    if test "x$PYTHON_FOUND" != "xyes"; then
        PKG_CHECK_MODULES(PYTHON, [python3], [PYTHON_FOUND=yes; PYTHON_VERSION=`$PKGCONFIG --modversion python3`], [PYTHON_FOUND=no])
    fi
    if test "x$PYTHON_FOUND" != "xyes" ; then
        AC_MSG_WARN([
*** Python must be installed on your system but python interpreter couldn't be found in path.
*** Please check that python is in path, or install it with your software package manager.
*** WeeChat will be built without Python support.])
        enable_python="no"
        not_found="$not_found python"
    fi
else
    not_asked="$not_asked python"
fi

if test "x$enable_python" = "xyes" ; then
    AC_SUBST(PYTHON_CFLAGS)
    AC_SUBST(PYTHON_LIBS)
    AC_SUBST(PYTHON_VERSION)
    AC_DEFINE(PLUGIN_PYTHON)
fi

# ---------------------------------- ruby --------------------------------------

RUBY_VERSION=

if test "x$enable_ruby" = "xyes" ; then
    RUBY_CFLAGS=""
    RUBY_LFLAGS=""
    for v in "3.2" "3.1" "3.0" "2.7" "2.6" "2.5" "2.4" "2.3" "2.2" "2.1" "2.0" "1.9" "1.8" ; do
        pkgconfig_ruby_found=`$PKGCONFIG --exists ruby-$v 2>/dev/null`
        if test "x$?" = "x0" ; then
            RUBY_VERSION=`$PKGCONFIG --modversion ruby-$v`
            RUBY_CFLAGS=`$PKGCONFIG --cflags ruby-$v`
            RUBY_LFLAGS=`$PKGCONFIG --libs ruby-$v`
            break
        fi
    done

    # detect old Ruby versions
    if test "x$RUBY_CFLAGS" = "x" -o "x$RUBY_LFLAGS" = "x" ; then
        AC_PATH_PROGS(RUBY, ruby1.9.3 ruby1.9.2 ruby1.9.1 ruby1.9 ruby)
        if test -z $RUBY ; then
            AC_MSG_WARN([
*** Ruby must be installed on your system but ruby interpreter couldn't be found in path.
*** Please check that ruby is in path, or install it with your software package manager.
*** WeeChat will be built without Ruby support.])
            enable_ruby="no"
            not_found="$not_found ruby"
        else
            RUBY_VERSION=`$RUBY -rrbconfig -e "puts RbConfig::CONFIG[['MAJOR']] + '.' + RbConfig::CONFIG[['MINOR']] + '.' + RbConfig::CONFIG[['TEENY']]"`
            RUBY_INCLUDE=`$RUBY -rrbconfig -e "puts RbConfig::CONFIG[['rubyhdrdir']] || RbConfig::CONFIG[['archdir']]"`
            RUBY_ARCH=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["arch"]]'`
            AC_MSG_CHECKING(for Ruby header files)
            if test -d "$RUBY_INCLUDE/"; then
                RUBY_CFLAGS="-I$RUBY_INCLUDE/ -I$RUBY_INCLUDE/$RUBY_ARCH"
            else
                AC_MSG_WARN([
*** Ruby header files couldn't be found on your system.
*** Try to install them with your software package manager.
*** WeeChat will be built without Ruby support.])
                enable_ruby="no"
                not_found="$not_found ruby"
            fi
            AC_MSG_RESULT(found)
            RUBY_LFLAGS=`$RUBY -rrbconfig -e "puts RbConfig::CONFIG[['LIBRUBYARG_SHARED']]"`
        fi
    fi
else
    not_asked="$not_asked ruby"
fi

if test "x$enable_ruby" = "xyes" ; then
    AC_SUBST(RUBY_CFLAGS)
    AC_SUBST(RUBY_LFLAGS)
    AC_DEFINE(PLUGIN_RUBY)
fi

# ---------------------------------- lua --------------------------------------

LUA_VERSION=

if test "x$enable_lua" = "xyes" ; then
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LDFLAGS="$LDFLAGS"

    LUA_CFLAGS=""
    LUA_LFLAGS=""

    if test -n "$lua_inc"; then
       CFLAGS="$CFLAGS -I$lua_inc"
       CXXFLAGS="$CXXFLAGS -I$lua_inc"
    fi
    if test -n "$lua_lib"; then
       LDFLAGS="$LDFLAGS -L$lua_lib"
    fi

    if test "x$LUA_CFLAGS" = "x" -o "x$LUA_LFLAGS" = "x" ; then
        AC_MSG_CHECKING(for Lua headers and libraries with pkg-config)
        echo
        for l in "54" "5.4" "53" "5.3" "52" "5.2" "51" "5.1" "50" "5.0" "$lua_suffix" "" ; do
            pkgconfig_lua_found=`$PKGCONFIG --exists lua$l 2>/dev/null`
            if test "x$?" = "x0" ; then
                LUA_VERSION=`$PKGCONFIG --modversion lua$l`
                LUA_CFLAGS="$LUA_CFLAGS "`$PKGCONFIG --cflags lua$l`
                LUA_LFLAGS="$LUA_LFLAGS "`$PKGCONFIG --libs lua$l`

                pkgconfig_lualib_found=`$PKGCONFIG --exists lualib$l 2>/dev/null`
                if test  "x$?" = "x0"; then
                    LUA_CFLAGS="$LUA_CFLAGS "`$PKGCONFIG --cflags lualib$l`
                    LUA_LFLAGS="$LUA_LFLAGS "`$PKGCONFIG --libs lualib$l`
                fi
                break
            fi
        done
    fi

    if test "x$LUA_CFLAGS" = "x" -o "x$LUA_LFLAGS" = "x" ; then
        LUACONFIG=""
        AC_CHECK_PROGS(LUACONFIG, lua-config54 lua-config5.4 lua-config53 lua-config5.3 lua-config52 lua-config5.2 lua-config51 lua-config5.1 lua-config50 lua-config5.0 lua-config)
        if test "x$LUACONFIG" != "x" ; then
            AC_MSG_CHECKING(for Lua headers and libraries with lua-config)
            echo
            LUA_CFLAGS=`$LUACONFIG --include`
            LUA_LFLAGS=`$LUACONFIG --libs`
            LUA_VERSION="5.0.x"
        fi
    fi

    if test "x$LUA_CFLAGS" = "x" -o "x$LUA_LFLAGS" = "x" ; then
        AC_MSG_CHECKING(for Lua headers and libraries)
        echo
        AC_CHECK_HEADER(lua.h,ac_found_lua_header="yes",ac_found_lua_header="no")
        AC_CHECK_HEADER(lualib.h,ac_found_liblua_header="yes",ac_found_liblua_header="no")
        if test "x$ac_found_lua_header" = "xyes" -a "x$ac_found_liblua_header" = "xyes"; then
            LUA_CFLAGS="$CFLAGS"
        fi
        for l in "54" "5.4" "53" "5.3" "52" "5.2" "51" "5.1" "50" "5.0" "$lua_suffix" "" ; do
            AC_CHECK_LIB(lua$l,lua_call,ac_found_lua_lib="yes",ac_found_lua_lib="no")
            if test "x$ac_found_lua_lib" = "xyes" ; then
                LUA_VERSION=">=5.1.0"

                LUA_LFLAGS="$LDFLAGS -llua$l -lm"

                ac2_save_LDFLAGS="$LDFLAGS"
                LDFLAGS="$LDFLAGS -llua$l -lm"

                if echo "$host_os" | grep "^linux" 1>/dev/null 2>&1 ; then
                    LDFLAGS="$LDFLAGS -ldl"
                fi

                AC_CHECK_LIB(lualib$l,luaL_openlib,ac_found_liblua_lib="yes",ac_found_liblua_lib="no")
                if test "x$ac_found_liblua_lib" = "xyes" ; then
                    LUA_VERSION="5.0.x"

                    LUA_LFLAGS="$LUA_LFLAGS -llualib$l"

                    if echo "$host_os" | grep "^linux" 1>/dev/null 2>&1 ; then
                        LUA_LFLAGS="$LUA_LFLAGS -ldl"
                    fi
                fi

                LDFLAGS="$ac2_save_LDFLAGS"
                break
            fi
        done
    fi

    AC_MSG_CHECKING(for Lua compiling and linking)
    LUA_TEST=`LT=luatest.c ; echo "#include <lua.h>" > $LT;  echo "#include <lualib.h>" >> $LT; echo "int main() { luaopen_base((lua_State *)lua_open()); return 0; }" >> $LT ; $CC -Wall $LT -o $LT.out $LUA_CFLAGS $LUA_LFLAGS $CFLAGS $LDFLAGS 1>/dev/null 2>&1 ; echo $?; rm -f $LT $LT.out 1>/dev/null 2>&1`
    if test "x$LUA_TEST" != "x0" ; then
        LUA52_TEST=`LT=luatest.c ; echo "#include <lua.h>" > $LT;  echo "#include <lualib.h>" >> $LT; echo "int main() { luaopen_base((lua_State *)luaL_newstate()); return 0; }" >> $LT ; $CC -Wall $LT -o $LT.out $LUA_CFLAGS $LUA_LFLAGS $CFLAGS $LDFLAGS 1>/dev/null 2>&1 ; echo $?; rm -f $LT $LT.out 1>/dev/null 2>&1`
        if test "x$LUA52_TEST" != "x0" ; then
            AC_MSG_RESULT(no)
            AC_MSG_WARN([
*** Lua (>=5.0) headers and/or libraries couldn't be found on your system.
*** Try to install liblua, liblualib and liblua-dev with your software package manager.
*** WeeChat will be built without Lua support.])
            enable_lua="no"
            not_found="$not_found lua"
        else
            AC_MSG_RESULT(yes)
        fi
    else
        AC_MSG_RESULT(yes)
    fi

    CFLAGS="$ac_save_CFLAGS"
    CXXFLAGS="$ac_save_CXXFLAGS"
    LDFLAGS="$ac_save_LDFLAGS"
else
    not_asked="$not_asked lua"
fi

if test "x$enable_lua" = "xyes" ; then
    AC_SUBST(LUA_CFLAGS)
    AC_SUBST(LUA_LFLAGS)
    AC_DEFINE(PLUGIN_LUA)
fi

# --------------------------------- tcl -------------------------------------

TCL_VERSION=

if test "x$enable_tcl" = "xyes" ; then
    enable_plugins="yes"

    AC_MSG_CHECKING(for tclConfig.sh)
    tcl_found="no"
    tcl_dirs="/usr/lib/tcl8.6 /usr/lib64/tcl8.6 /usr/lib/tcl8.5 /usr/lib64/tcl8.5 /lib /lib64 /usr/lib /usr/lib64 /usr/tcl/lib /usr/tcl/lib64 /usr/local/tcl-8.6/lib /usr/local/tcl-8.6/lib64 /usr/local/tcl-8.5/lib /usr/local/tcl-8.5/lib64 /usr/local/lib /usr/local/lib64 /usr/local/tcl/lib /usr/local/tcl/lib64 /opt/lib /opt/lib64"
    if test "x$tclconfig" != "x" ; then
      tcl_dirs="${tclconfig} ${tcl_dirs}"
    fi
    for tcl_dir in $tcl_dirs ; do
        if test -f ${tcl_dir}/tclConfig.sh ; then
            . ${tcl_dir}/tclConfig.sh
            TCL_CFLAGS="-I${TCL_PREFIX}/include $TCL_INCLUDE_SPEC"
            TCL_LFLAGS="$TCL_LIB_SPEC $TCL_LIBS"
            tcl_found="yes"
            AC_MSG_RESULT(${tcl_dir}/tclConfig.sh)
            break
        fi
    done
    if test "x$tcl_found" = "xno" ; then
        AC_MSG_WARN([
*** Script tclConfig.sh couldn't be found on your system.
*** WeeChat will be built without Tcl support.])
        enable_tcl="no"
        not_found="$not_found tcl"
    fi
else
    not_asked="$not_asked tcl"
fi

if test "x$enable_tcl" = "xyes" ; then
    AC_SUBST(TCL_CFLAGS)
    AC_SUBST(TCL_LFLAGS)
    AC_DEFINE(PLUGIN_TCL)
fi

# --------------------------------- guile -------------------------------------

GUILE_VERSION=

if test "x$enable_guile" = "xyes" ; then
    enable_plugins="yes"

    guile_found="no"
    AC_MSG_CHECKING(for Guile headers and libraries)
    echo
    for v in "3.0" "2.2" "2.0" ; do
        pkgconfig_guile_found=`$PKGCONFIG --exists guile-$v 2>/dev/null`
        if test "x$?" = "x0" ; then
            GUILE_VERSION=`$PKGCONFIG --modversion guile-$v`
            GUILE_CFLAGS=`$PKGCONFIG --cflags guile-$v`
            GUILE_LFLAGS=`$PKGCONFIG --libs guile-$v`
            guile_found="yes"
            break
        fi
    done

    if test "x$guile_found" = "xyes" ; then
        # check if variable "scm_install_gmp_memory_functions" exists
        ac_save_CFLAGS="$CFLAGS"
        ac_save_LDFLAGS="$LDFLAGS"
        CFLAGS="$CFLAGS $GUILE_CFLAGS"
        LDFLAGS="$LDFLAGS $GUILE_LFLAGS"
        AC_CACHE_CHECK([for variable scm_install_gmp_memory_functions], ac_cv_have_guile_gmp_memory_functions, [
        AC_LINK_IFELSE([AC_LANG_PROGRAM(
            [[ #include <libguile.h>]],
            [[ scm_install_gmp_memory_functions = 0; ]])],
            [ ac_have_guile_gmp_memory_functions="yes" ],
            [ ac_have_guile_gmp_memory_functions="no" ])])
        CFLAGS="$ac_save_CFLAGS"
        LDFLAGS="$ac_save_LDFLAGS"
        if test "x$ac_have_guile_gmp_memory_functions" = "xyes"; then
            AC_DEFINE(HAVE_GUILE_GMP_MEMORY_FUNCTIONS)
        fi
    else
        AC_MSG_WARN([
*** Guile >= 2.0 couldn't be found on your system.
*** WeeChat will be built without Guile (scheme) support.])
        enable_guile="no"
        not_found="$not_found guile"
    fi
else
    not_asked="$not_asked guile"
fi

if test "x$enable_guile" = "xyes" ; then
    AC_SUBST(GUILE_CFLAGS)
    AC_SUBST(GUILE_LFLAGS)
    AC_DEFINE(PLUGIN_GUILE)
fi

# ------------------------------ javascript -----------------------------------

AC_LANG_PUSH([C++])

if test "x$enable_javascript" = "xyes" ; then
    enable_plugins="yes"

    v8_found="no"
    AC_CHECK_HEADER(v8.h,ac_found_v8_header="yes",ac_found_v8_header="no")
    if test "x$ac_found_v8_header" = "xyes" ; then
        #AC_CHECK_LIB(v8,v8,ac_found_v8_lib="yes",ac_found_v8_lib="no")
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS -lv8 -lpthread"
        AC_MSG_CHECKING(for v8 usability in programs)
        AC_TRY_RUN([
            #include <v8.h>
            using namespace v8;
            int main(int argc, char **argv) {
                const char *version = V8::GetVersion();
                return 0;
            }],ac_found_v8_lib="yes", ac_found_v8_lib="no", ac_found_v8_lib="assume-yes")
        LIBS="$ac_save_LIBS"
        if test "x$ac_found_v8_lib" = "xyes" ; then
            AC_MSG_RESULT(yes)
            v8_found="yes"
            V8_CFLAGS=""
            V8_LFLAGS="-lv8 -lpthread"
        else
            AC_MSG_RESULT(no)
            AC_MSG_WARN([
*** V8 library couldn't be found on your system.
*** WeeChat will be built without JavaScript support.])
            enable_javascript="no"
            not_found="$not_found javascript/v8"
        fi
    else
        AC_MSG_WARN([
*** V8 header files couldn't be found on your system.
*** WeeChat will be built without JavaScript support.])
        enable_javascript="no"
        not_found="$not_found javascript/v8"
    fi
else
    not_asked="$not_asked javascript"
fi

if test "x$enable_javascript" = "xyes" ; then
    AC_SUBST(V8_CFLAGS)
    AC_SUBST(V8_LFLAGS)
    AC_DEFINE(PLUGIN_JAVASCRIPT)
fi

AC_LANG_POP

# ---------------------------------- php --------------------------------------

PHP_VERSION=

if test "x$enable_php" = "xyes" ; then
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LDFLAGS="$LDFLAGS"

    PHP_CFLAGS=""
    PHP_LFLAGS=""
    PHP_VERSIONS="8.2 82 8.1 81 8.0 80 8 7.4 74 7.3 73 7.2 72 7.1 71 7.0 70 7 $php_suffix"

    if test -n "$php_inc"; then
       CFLAGS="$CFLAGS -I$php_inc"
       CXXFLAGS="$CXXFLAGS -I$php_inc"
    fi
    if test -n "$php_lib"; then
       LDFLAGS="$LDFLAGS -L$php_lib"
    fi

    if test "x$PHP_CFLAGS" = "x" -o "x$PHP_LFLAGS" = "x" ; then
        AC_MSG_CHECKING(for PHP headers and libraries with pkg-config)
        echo
        for l in $PHP_VERSIONS "" ; do
            pkgconfig_php_found=`$PKGCONFIG --exists php$l 2>/dev/null`
            if test "x$?" = "x0" ; then
                pkgconfig_php_found=`$PKGCONFIG --atleast-version=7 php$l 2>/dev/null`
                if test "x$?" = "x0" ; then
                    PHP_VERSION=`$PKGCONFIG --modversion php$l`
                    PHP_CFLAGS="$PHP_CFLAGS "`$PKGCONFIG --cflags php$l`
                    PHP_LFLAGS="$PHP_LFLAGS "`$PKGCONFIG --libs php$l`
                    break
                fi
            fi
        done
    fi

    if test "x$PHP_CFLAGS" = "x" -o "x$PHP_LFLAGS" = "x" ; then
        PHPCONFIG=""
        AC_MSG_CHECKING(for PHP headers and libraries with php-config)
        echo
        for l in $PHP_VERSIONS "" ; do
            AC_CHECK_PROG(PHPCONFIG, "php-config$l", "php-config$l")
            if test "x$PHPCONFIG" != "x" ; then
                php_config_version=`$PHPCONFIG --version`
                if echo "x$php_config_version" | grep -e "^x7" -e "^x8" 1>/dev/null 2>&1 ; then
                    PHP_VERSION=$php_config_version
                    PHP_CFLAGS=`$PHPCONFIG --includes`
                    PHP_LFLAGS="-L$($PHPCONFIG --prefix)/lib/ $($PHPCONFIG --libs) -lphp$l"
                    break
                fi
            fi
        done
    fi

    if test "x$PHP_CFLAGS" = "x" -o "x$PHP_LFLAGS" = "x" ; then
        AC_MSG_CHECKING(for PHP headers and libraries)
        echo
        AC_CHECK_HEADER(php.h,ac_found_php_header="yes",ac_found_php_header="no")
        if test "x$ac_found_php_header" = "xyes" ; then
            PHP_CFLAGS="$CFLAGS"
        fi
        for l in $PHP_VERSIONS "" ; do
            for PHP_LIB_SUFFIX in "$l" "$(echo $l | cut -c1)" "" ; do
                AC_CHECK_LIB(php$PHP_LIB_SUFFIX,php_execute_script,ac_found_php_lib="yes",ac_found_php_lib="no")
                if test "x$ac_found_php_lib" = "xyes" ; then
                    PHP_VERSION=">=7.0.0"

                    PHP_LFLAGS="$LDFLAGS -lphp$PHP_LIB_SUFFIX -lm"

                    ac2_save_LDFLAGS="$LDFLAGS"
                    LDFLAGS="$LDFLAGS -lphp$PHP_LIB_SUFFIX -lm"

                    if echo "$host_os" | grep "^linux" 1>/dev/null 2>&1 ; then
                        LDFLAGS="$LDFLAGS -ldl"
                    fi

                    LDFLAGS="$ac2_save_LDFLAGS"
                    break 2
                fi
            done
        done
    fi

    AC_MSG_CHECKING(for PHP compiling and linking)
    PHP_TEST=`LT=phptest.c ; echo "#include <sapi/embed/php_embed.h>" > $LT; echo "int main() { php_embed_init(0, NULL); php_embed_shutdown(); return 0; }" >> $LT ; $CC -Wall $LT -o $LT.out $PHP_CFLAGS $PHP_LFLAGS $CFLAGS $LDFLAGS 1>/dev/null 2>&1 ; echo $?; rm -f $LT $LT.out 1>/dev/null 2>&1`
    if test "x$PHP_TEST" != "x0" ; then
        AC_MSG_RESULT(no)
        AC_MSG_WARN([
*** PHP (>=7.0) headers and/or libraries couldn't be found on your system.
*** Try to install libphp-embed with your software package manager.
*** WeeChat will be built without PHP support.])
        enable_php="no"
        not_found="$not_found php"
    else
        AC_MSG_RESULT(yes)
    fi

    CFLAGS="$ac_save_CFLAGS"
    CXXFLAGS="$ac_save_CXXFLAGS"
    LDFLAGS="$ac_save_LDFLAGS"
else
    not_asked="$not_asked php"
fi

if test "x$enable_php" = "xyes" ; then
    AC_SUBST(PHP_CFLAGS)
    AC_SUBST(PHP_LFLAGS)
    AC_DEFINE(PLUGIN_PHP)
fi

# ----------------------------------- spell ------------------------------------

if test "x$enable_spell" = "xyes" ; then
    ac_found_enchant_lib="no"
    if test "x$enable_enchant" = "xyes" ; then
        PKG_CHECK_MODULES(ENCHANT, [enchant], ac_found_enchant_lib="yes", ac_found_enchant_lib="no")
    fi
    if test "x$ac_found_enchant_lib" = "xyes" ; then
        CFLAGS="$CFLAGS -DUSE_ENCHANT"
        SPELL_LIB_USED="enchant"

        # check if function enchant_get_version() exists
        ac_save_CFLAGS="$CFLAGS"
        ac_save_LDFLAGS="$LDFLAGS"
        CFLAGS="$CFLAGS $ENCHANT_CFLAGS"
        LDFLAGS="$LDFLAGS $ENCHANT_LIBS"
        AC_CACHE_CHECK([for enchant_get_version() support], ac_cv_have_enchant_get_version, [
        AC_LINK_IFELSE([AC_LANG_PROGRAM(
            [[ #include <enchant.h>]],
            [[ const char *version = enchant_get_version(); ]])],
            [ ac_have_enchant_get_version="yes" ],
            [ ac_have_enchant_get_version="no" ])])
        CFLAGS="$ac_save_CFLAGS"
        LDFLAGS="$ac_save_LDFLAGS"
        if test "x$ac_have_enchant_get_version" = "xyes"; then
            AC_DEFINE(HAVE_ENCHANT_GET_VERSION)
        fi
    else
        ASPELL_CFLAGS=""
        ASPELL_LFLAGS=""

        AC_CHECK_HEADER(aspell.h,ac_found_aspell_header="yes",ac_found_aspell_header="no")
        AC_CHECK_LIB(aspell,new_aspell_speller,ac_found_aspell_lib="yes",ac_found_aspell_lib="no")

        AC_MSG_CHECKING(for aspell headers and libraries)
        if test "x$ac_found_aspell_header" = "xno" -o "x$ac_found_aspell_lib" = "xno" ; then
            AC_MSG_RESULT(no)
            AC_MSG_WARN([
*** Aspell headers and/or libraries couldn't be found on your system.
*** Try to install them with your software package manager.
*** WeeChat will be built without Aspell support.])
            enable_spell="no"
            not_found="$not_found spell"
        else
            AC_MSG_RESULT(yes)
            ASPELL_LFLAGS="$ASPELL_LFLAGS -laspell"
            SPELL_LIB_USED="aspell"

            # check if function aspell_version_string() exists
            ac_save_LDFLAGS="$LDFLAGS"
            LDFLAGS="$LDFLAGS $ASPELL_LFLAGS"
            AC_CACHE_CHECK([for aspell_version_string() support], ac_cv_have_aspell_version_string, [
            AC_LINK_IFELSE([AC_LANG_PROGRAM(
                [[ #include <aspell.h>]],
                [[ const char *version = aspell_version_string(); ]])],
                [ ac_have_aspell_version_string="yes" ],
                [ ac_have_aspell_version_string="no" ])])
            LDFLAGS="$ac_save_LDFLAGS"
            if test "x$ac_have_aspell_version_string" = "xyes"; then
                AC_DEFINE(HAVE_ASPELL_VERSION_STRING)
            fi
        fi
    fi
else
    not_asked="$not_asked spell"
fi

if test "x$enable_spell" = "xyes" ; then
    AC_SUBST(ASPELL_CFLAGS)
    AC_SUBST(ASPELL_LFLAGS)
    AC_DEFINE(PLUGIN_SPELL)
fi

# --------------------------------- trigger ------------------------------------

if test "x$enable_trigger" = "xyes" ; then
    TRIGGER_CFLAGS=""
    TRIGGER_LFLAGS=""
    AC_SUBST(TRIGGER_CFLAGS)
    AC_SUBST(TRIGGER_LFLAGS)
    AC_DEFINE(PLUGIN_TRIGGER)
else
    not_asked="$not_asked trigger"
fi

# ---------------------------------- typing ------------------------------------

if test "x$enable_typing" = "xyes" ; then
    TYPING_CFLAGS=""
    TYPING_LFLAGS=""
    AC_SUBST(TYPING_CFLAGS)
    AC_SUBST(TYPING_LFLAGS)
    AC_DEFINE(PLUGIN_TYPING)
else
    not_asked="$not_asked typing"
fi

# ---------------------------------- xfer --------------------------------------

if test "x$enable_xfer" = "xyes" ; then
    XFER_CFLAGS=""
    XFER_LFLAGS=""
    AC_SUBST(XFER_CFLAGS)
    AC_SUBST(XFER_LFLAGS)
    AC_DEFINE(PLUGIN_XFER)
else
    not_asked="$not_asked xfer"
fi

# ------------------------------------------------------------------------------
#                                   gcrypt
# ------------------------------------------------------------------------------

AC_CHECK_HEADER(gcrypt.h,ac_found_gcrypt_header="yes",ac_found_gcrypt_header="no")
AC_CHECK_LIB(gcrypt,gcry_check_version,ac_found_gcrypt_lib="yes",ac_found_gcrypt_lib="no")

AC_MSG_CHECKING(for gcrypt headers and libraries)
if test "x$ac_found_gcrypt_header" = "xno" -o "x$ac_found_gcrypt_lib" = "xno" ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([
*** libgcrypt was not found. You may want to get it from https://www.gnupg.org/ftp/gcrypt/libgcrypt/
*** or try to install it with your software package manager.])
else
    AC_MSG_RESULT(yes)
    GCRYPT_CFLAGS=`libgcrypt-config --cflags`
    GCRYPT_LFLAGS=`libgcrypt-config --libs`
    AC_SUBST(GCRYPT_CFLAGS)
    AC_SUBST(GCRYPT_LFLAGS)
fi

# ------------------------------------------------------------------------------
#                                   gnutls
# ------------------------------------------------------------------------------

AC_CHECK_HEADER(gnutls/gnutls.h,ac_found_gnutls_header="yes",ac_found_gnutls_header="no")
AC_CHECK_LIB(gnutls,gnutls_global_init,ac_found_gnutls_lib="yes",ac_found_gnutls_lib="no")

AC_MSG_CHECKING(for gnutls headers and libraries)
if test "x$ac_found_gnutls_header" = "xno" -o "x$ac_found_gnutls_lib" = "xno" ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([
*** libgnutls was not found. You may want to get it from https://www.gnupg.org/ftp/gcrypt/gnutls/
*** or try to install it with your software package manager.])
else
    AC_MSG_RESULT(yes)
    GNUTLS_CFLAGS=`pkg-config gnutls --cflags`
    GNUTLS_LFLAGS=`pkg-config gnutls --libs`
    AC_SUBST(GNUTLS_CFLAGS)
    AC_SUBST(GNUTLS_LFLAGS)
fi

# ------------------------------------------------------------------------------
#                                   flock
# ------------------------------------------------------------------------------

enable_flock="no"
AC_CACHE_CHECK([for flock() support], ac_cv_have_flock, [
AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[ #include <sys/file.h>]],
[[ flock(0, LOCK_SH); ]])],
[ ac_cv_have_flock="yes" ],
[ ac_cv_have_flock="no" ])])

if test "x$ac_cv_have_flock" = "xyes"; then
    enable_flock="yes"
    AC_DEFINE(HAVE_FLOCK)
else
    not_found="$not_found flock"
fi

# ------------------------------------------------------------------------------
#                               large file support
# ------------------------------------------------------------------------------

if test "x$enable_largefile" = "xyes" ; then
   CFLAGS="$CFLAGS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D_LARGE_FILES"
   CXXFLAGS="$CXXFLAGS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D_LARGE_FILES"
else
   not_asked="$not_asked largefile"
fi

# ------------------------------------------------------------------------------
#                                  backtrace
# ------------------------------------------------------------------------------

enable_backtrace="no"
if test "x$debug" != "x0" ; then
    AC_CACHE_CHECK([for execinfo.h and backtrace], ac_cv_have_backtrace, [
        AC_LINK_IFELSE([AC_LANG_PROGRAM(
            [[ #include <execinfo.h> ]],
            [[ void *trace[128]; int n = backtrace(trace, 128); ]])],
            [ ac_cv_have_backtrace="yes" ],
            [ ac_cv_have_backtrace="no"  ])])
    if test "x$ac_cv_have_backtrace" = "xyes"; then
        enable_backtrace="yes"
        AC_DEFINE(HAVE_BACKTRACE,1,[glibc backtrace function])
    else
        not_found="$not_found backtrace"
    fi
fi

# ------------------------------------------------------------------------------
#                             eat_newline_glitch
# ------------------------------------------------------------------------------

enable_eatnewlineglitch="no"
AC_CACHE_CHECK([for eat_newline_glitch support], ac_cv_have_eatnewlineglitch, [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[ #include <term.h> ]],
[[ eat_newline_glitch = 0; ]])],
[ ac_cv_have_eatnewlineglitch="yes" ],
[ ac_cv_have_eatnewlineglitch="no" ])])

if test "x$ac_cv_have_eatnewlineglitch" = "xyes"; then
    enable_eatnewlineglitch="yes"
    AC_DEFINE(HAVE_EAT_NEWLINE_GLITCH)
else
    not_found="$not_found eat_newline_glitch"
fi

# ------------------------------------------------------------------------------
#                                     zlib
# ------------------------------------------------------------------------------

AC_CHECK_HEADER(zlib.h,ac_found_zlib_header="yes",ac_found_zlib_header="no")
AC_CHECK_LIB(z,compress2,ac_found_zlib_lib="yes",ac_found_zlib_lib="no")

AC_MSG_CHECKING(for zlib headers and libraries)
if test "x$ac_found_zlib_header" = "xno" -o "x$ac_found_zlib_lib" = "xno" ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([
*** zlib was not found. You may want to get it from https://zlib.net/
*** or try to install it with your software package manager.])
else
    AC_MSG_RESULT(yes)
    ZLIB_CFLAGS=`pkg-config zlib --cflags`
    ZLIB_LFLAGS=`pkg-config zlib --libs`
    AC_SUBST(ZLIB_CFLAGS)
    AC_SUBST(ZLIB_LFLAGS)
fi

# ------------------------------------------------------------------------------
#                                     zstd
# ------------------------------------------------------------------------------

AC_CHECK_HEADER(zstd.h,ac_found_zstd_header="yes",ac_found_zstd_header="no")
AC_CHECK_LIB(zstd,ZSTD_compress,ac_found_zstd_lib="yes",ac_found_zstd_lib="no")

AC_MSG_CHECKING(for zstd headers and libraries)
if test "x$ac_found_zstd_header" = "xno" -o "x$ac_found_zstd_lib" = "xno" ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([
*** zstd was not found. You may want to get it from https://github.com/facebook/zstd
*** or try to install it with your software package manager.])
else
    AC_MSG_RESULT(yes)
    ZSTD_CFLAGS=`pkg-config libzstd --cflags`
    ZSTD_LFLAGS=`pkg-config libzstd --libs`
    AC_SUBST(ZSTD_CFLAGS)
    AC_SUBST(ZSTD_LFLAGS)
fi

# ------------------------------------------------------------------------------
#                                     curl
# ------------------------------------------------------------------------------

AC_PATH_PROG(CURL_CONFIG, curl-config)
if test "x$CURL_CONFIG" = "x" ; then
    AC_MSG_ERROR([
*** "curl-config" couldn't be found on your system.
*** Try to install libcurl-dev with your software package manager.])
fi
CURL_CFLAGS=`$CURL_CONFIG --cflags`
CURL_LFLAGS=`$CURL_CONFIG --libs`
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LFLAGS)

# ------------------------------------------------------------------------------
#                                    tests
# ------------------------------------------------------------------------------

if test "x$enable_tests" = "xyes" ; then
    cpputest_found="no"
    AC_MSG_CHECKING(for CppUTest headers and libraries)
    echo
    pkgconfig_cpputest_found=`$PKGCONFIG --exists cpputest 2>/dev/null`
    if test "x$?" = "x0" ; then
        CPPUTEST_VERSION=`$PKGCONFIG --modversion cpputest`
        CPPUTEST_CFLAGS=`$PKGCONFIG --cflags cpputest`
        CPPUTEST_LFLAGS=`$PKGCONFIG --libs cpputest`
    else
        AC_MSG_WARN([
*** CppUTest couldn't be found on your system.
*** WeeChat will be built without tests.])
        enable_tests="no"
        not_found="$not_found tests"
    fi
else
    not_asked="$not_asked tests"
fi

if test "x$enable_tests" = "xyes" ; then
    AC_SUBST(CPPUTEST_CFLAGS)
    AC_SUBST(CPPUTEST_LFLAGS)
    AC_DEFINE(TESTS)
fi

# ------------------------------------------------------------------------------
#                           man page / documentation
# ------------------------------------------------------------------------------

msg_man=""
msg_doc=""

if test "x$enable_man" = "xno"; then
    not_asked="$not_asked man"
fi
if test "x$enable_doc" = "xno"; then
    not_asked="$not_asked doc"
fi

if test "x$enable_man" = "xyes" -o "x$enable_doc" = "xyes"; then
    AC_CHECK_PROGS(ASCIIDOCTOR, [asciidoctor])
    if test -n "$ASCIIDOCTOR"; then
        AC_MSG_CHECKING([for asciidoctor version])
        asciidoctor_version=`$ASCIIDOCTOR --version 2>/dev/null | head -n 1 | cut -d" " -f2`
        case "${asciidoctor_version}" in
        Asciidoctor' '0.*|Asciidoctor' '1.5.[0-3]*)
                AC_MSG_RESULT([${asciidoctor_version} (too old)])
                not_found="$not_found asciidoctor"
                enable_man="no"
                enable_doc="no"
                ;;
        *)
                AC_MSG_RESULT([${asciidoctor_version}])
                if test "x$enable_man" = "xyes"; then
                    msg_man="asciidoctor(${asciidoctor_version}) $msg_man"
                    AC_DEFINE(MAN)
                fi
                if test "x$enable_doc" = "xyes"; then
                    msg_doc="asciidoctor(${asciidoctor_version}) $msg_doc"
                    AC_DEFINE(DOC)
                fi
                ;;
        esac
    else
        not_found="$not_found asciidoctor"
        enable_man="no"
        enable_doc="no"
    fi
    ASCIIDOCTOR_ARGS="-a experimental -a reproducible -a 'prewrap!' -a 'webfonts!' -a icons=font -a sectanchors -a source-highlighter=pygments -a pygments-style=native"
    AC_SUBST(ASCIIDOCTOR)
    AC_SUBST(ASCIIDOCTOR_ARGS)
fi

# ------------------------------------------------------------------------------
#                                general vars
# ------------------------------------------------------------------------------

if test "x$prefix" = "xNONE" ; then
    prefix="$ac_default_prefix"
fi

if test "x$exec_prefix" = "xNONE" ; then
    exec_prefix="$prefix"
fi

AC_DEFINE_UNQUOTED(PREFIX, "${prefix}")

WEECHAT_LIBDIR=`eval eval echo ${libdir}/weechat`
AC_DEFINE_UNQUOTED(WEECHAT_LIBDIR, "$WEECHAT_LIBDIR")

WEECHAT_SHAREDIR=`eval eval echo ${datadir}/weechat`
AC_DEFINE_UNQUOTED(WEECHAT_SHAREDIR, "$WEECHAT_SHAREDIR")

weechat_libdir=${libdir}/weechat
AC_SUBST(weechat_libdir)

COMMON_CFLAGS="-fsigned-char -Wall -Wextra -Werror-implicit-function-declaration"
COMMON_CXXFLAGS="-fsigned-char -Wall -Wextra"

AC_MSG_CHECKING([whether we have GNU assembler])
GAS=`as --version < /dev/null 2>/dev/null | grep GNU`
if test "$GAS"; then
    COMMON_CFLAGS="${COMMON_CFLAGS} -pipe"
    COMMON_CXXFLAGS="${COMMON_CXXFLAGS} -pipe"
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi

CFLAGS=`echo $CFLAGS | sed 's/ -g / /g'`
CFLAGS=`echo $CFLAGS | sed 's/^-g //g'`
CFLAGS=`echo $CFLAGS | sed 's/ -g$//g'`
CFLAGS=`echo $CFLAGS | sed 's/^-g$//g'`

CXXFLAGS=`echo $CXXFLAGS | sed 's/ -g / /g'`
CXXFLAGS=`echo $CXXFLAGS | sed 's/^-g //g'`
CXXFLAGS=`echo $CXXFLAGS | sed 's/ -g$//g'`
CXXFLAGS=`echo $CXXFLAGS | sed 's/^-g$//g'`

if test "x$debug" = "x0" ; then
    CFLAGS="$COMMON_CFLAGS $CFLAGS"
    CXXFLAGS="$COMMON_CXXFLAGS $CXXFLAGS"
else
    CFLAGS="$COMMON_CFLAGS $CFLAGS -g -O0"
    CXXFLAGS="$COMMON_CXXFLAGS $CXXFLAGS -g -O0"
fi

LIBS="$LIBS $INTLLIBS"

case "$host_os" in
freebsd*)
        if test "x$enable_perl" = "xyes" -o "x$enable_python" = "xyes" ; then
            CFLAGS="$CFLAGS -pthread"
        fi
        CFLAGS="$CFLAGS $CXXFLAGS"
        ;;
openbsd*)
        if test "x$enable_python" = "xyes" ; then
            CFLAGS="$CFLAGS -pthread"
        fi
        ;;
netbsd*)
        if test "x$enable_perl" = "xyes" -o "x$enable_python" = "xyes" ; then
            CFLAGS="$CFLAGS -pthread"
        fi
        CFLAGS="$CFLAGS $CXXFLAGS"
        ;;
gnu*)
        LDFLAGS="$LDFLAGS -lpthread"
        ;;
haiku*)
        LDFLAGS="$LDFLAGS -lnetwork"
        ;;
*)
        ;;
esac

CFLAGS="$CFLAGS -DWEECHAT_VERSION=\\\"$VERSION\\\" -DWEECHAT_LICENSE=\\\"$LICENSE\\\""
CXXFLAGS="$CXXFLAGS -DWEECHAT_VERSION=\\\"$VERSION\\\" -DWEECHAT_LICENSE=\\\"$LICENSE\\\""

# ------------------------------------------------------------------------------
#                                output Makefiles
# ------------------------------------------------------------------------------

AM_CONDITIONAL(HAVE_FLOCK,              test "$enable_flock" = "yes")
AM_CONDITIONAL(HAVE_EAT_NEWLINE_GLITCH, test "$enable_eatnewlineglitch" = "yes")
AM_CONDITIONAL(GUI_NCURSES,             test "$enable_ncurses" = "yes")
AM_CONDITIONAL(HEADLESS,                test "$enable_headless" = "yes")
AM_CONDITIONAL(PLUGIN_ALIAS,            test "$enable_alias" = "yes")
AM_CONDITIONAL(PLUGIN_BUFLIST,          test "$enable_buflist" = "yes")
AM_CONDITIONAL(PLUGIN_CHARSET,          test "$enable_charset" = "yes")
AM_CONDITIONAL(PLUGIN_EXEC,             test "$enable_exec" = "yes")
AM_CONDITIONAL(PLUGIN_FIFO,             test "$enable_fifo" = "yes")
AM_CONDITIONAL(PLUGIN_FSET,             test "$enable_fset" = "yes")
AM_CONDITIONAL(PLUGIN_IRC,              test "$enable_irc" = "yes")
AM_CONDITIONAL(PLUGIN_LOGGER,           test "$enable_logger" = "yes")
AM_CONDITIONAL(PLUGIN_RELAY,            test "$enable_relay" = "yes")
AM_CONDITIONAL(PLUGIN_SCRIPT,           test "$enable_script" = "yes")
AM_CONDITIONAL(PLUGIN_PERL,             test "$enable_perl" = "yes")
AM_CONDITIONAL(PLUGIN_PYTHON,           test "$enable_python" = "yes")
AM_CONDITIONAL(PLUGIN_RUBY,             test "$enable_ruby" = "yes")
AM_CONDITIONAL(PLUGIN_LUA,              test "$enable_lua" = "yes")
AM_CONDITIONAL(PLUGIN_TCL,              test "$enable_tcl" = "yes")
AM_CONDITIONAL(PLUGIN_GUILE,            test "$enable_guile" = "yes")
AM_CONDITIONAL(PLUGIN_JAVASCRIPT,       test "$enable_javascript" = "yes")
AM_CONDITIONAL(PLUGIN_PHP,              test "$enable_php" = "yes")
AM_CONDITIONAL(PLUGIN_SPELL,            test "$enable_spell" = "yes")
AM_CONDITIONAL(PLUGIN_TRIGGER,          test "$enable_trigger" = "yes")
AM_CONDITIONAL(PLUGIN_TYPING,           test "$enable_typing" = "yes")
AM_CONDITIONAL(PLUGIN_XFER,             test "$enable_xfer" = "yes")
AM_CONDITIONAL(TESTS,                   test "$enable_tests" = "yes")
AM_CONDITIONAL(MAN,                     test "$enable_man" = "yes")
AM_CONDITIONAL(DOC,                     test "$enable_doc" = "yes")

AC_OUTPUT([Makefile
           icons/Makefile
           doc/Makefile
           doc/cs/Makefile
           doc/de/Makefile
           doc/en/Makefile
           doc/es/Makefile
           doc/fr/Makefile
           doc/it/Makefile
           doc/ja/Makefile
           doc/pl/Makefile
           doc/ru/Makefile
           doc/sr/Makefile
           src/Makefile
           src/core/Makefile
           src/plugins/Makefile
           src/plugins/alias/Makefile
           src/plugins/buflist/Makefile
           src/plugins/charset/Makefile
           src/plugins/exec/Makefile
           src/plugins/fifo/Makefile
           src/plugins/fset/Makefile
           src/plugins/irc/Makefile
           src/plugins/logger/Makefile
           src/plugins/relay/Makefile
           src/plugins/script/Makefile
           src/plugins/perl/Makefile
           src/plugins/python/Makefile
           src/plugins/ruby/Makefile
           src/plugins/lua/Makefile
           src/plugins/tcl/Makefile
           src/plugins/guile/Makefile
           src/plugins/javascript/Makefile
           src/plugins/php/Makefile
           src/plugins/spell/Makefile
           src/plugins/trigger/Makefile
           src/plugins/typing/Makefile
           src/plugins/xfer/Makefile
           src/gui/Makefile
           src/gui/curses/Makefile
           src/gui/curses/normal/Makefile
           src/gui/curses/headless/Makefile
           tests/Makefile
           intl/Makefile
           po/Makefile.in])

# ------------------------------------------------------------------------------
#                                  end message
# ------------------------------------------------------------------------------

listgui=""
if test "x$enable_ncurses" = "xyes" ; then
    listgui="$listgui ncurses"
fi
if test "x$enable_headless" = "xyes" ; then
    listgui="$listgui headless"
fi

if test "x$listgui" = "x" ; then
    AC_MSG_ERROR([
*** No interface specified...
*** Please enable at least ncurses or headless.])
fi

listplugins=""
if test "x$enable_alias" = "xyes"; then
    listplugins="$listplugins alias"
fi
if test "x$enable_buflist" = "xyes"; then
    listplugins="$listplugins buflist"
fi
if test "x$enable_charset" = "xyes"; then
    listplugins="$listplugins charset"
fi
if test "x$enable_exec" = "xyes"; then
    listplugins="$listplugins exec"
fi
if test "x$enable_fifo" = "xyes"; then
    listplugins="$listplugins fifo"
fi
if test "x$enable_fset" = "xyes"; then
    listplugins="$listplugins fset"
fi
if test "x$enable_irc" = "xyes" ; then
    listplugins="$listplugins irc"
fi
if test "x$enable_logger" = "xyes"; then
    listplugins="$listplugins logger"
fi
if test "x$enable_relay" = "xyes"; then
    listplugins="$listplugins relay"
fi
if test "x$enable_script" = "xyes"; then
    listplugins="$listplugins script"
fi
if test "x$enable_perl" = "xyes"; then
    listplugins="$listplugins perl($PERL_VERSION)"
fi
if test "x$enable_python" = "xyes"; then
    listplugins="$listplugins python($PYTHON_VERSION)"
fi
if test "x$enable_ruby" = "xyes"; then
    listplugins="$listplugins ruby($RUBY_VERSION)"
fi
if test "x$enable_lua" = "xyes"; then
    listplugins="$listplugins lua($LUA_VERSION)"
fi
if test "x$enable_tcl" = "xyes"; then
    listplugins="$listplugins tcl($TCL_VERSION)"
fi
if test "x$enable_guile" = "xyes"; then
    listplugins="$listplugins guile($GUILE_VERSION)"
fi
if test "x$enable_javascript" = "xyes"; then
    listplugins="$listplugins javascript/v8"
fi
if test "x$enable_php" = "xyes"; then
    listplugins="$listplugins php($PHP_VERSION)"
fi
if test "x$enable_spell" = "xyes"; then
    listplugins="$listplugins spell($SPELL_LIB_USED)"
fi
if test "x$enable_trigger" = "xyes"; then
    listplugins="$listplugins trigger"
fi
if test "x$enable_typing" = "xyes"; then
    listplugins="$listplugins typing"
fi
if test "x$enable_xfer" = "xyes"; then
    listplugins="$listplugins xfer"
fi

listoptional=""
if test "x$enable_flock" = "xyes"; then
    listoptional="$listoptional flock"
fi
if test "x$enable_largefile" = "xyes"; then
    listoptional="$listoptional largefile"
fi
if test "x$enable_backtrace" = "xyes"; then
    listoptional="$listoptional backtrace"
fi

msg_debug="no"
if test "x$debug" != "x0"; then
    msg_debug="yes"
fi

msg_tests="no"
if test "x$enable_tests" = "xyes"; then
    msg_tests="yes"
fi

if test "x$msg_man" = "x"; then
    msg_man="no"
else
    msg_man="yes: $msg_man"
fi
if test "x$msg_doc" = "x"; then
    msg_doc="no"
else
    msg_doc="yes: $msg_doc"
fi

echo ""
echo "Enabled features:"
echo "   Interfaces............. :$listgui"
echo "   Plugins................ :$listplugins"
echo "   Optional features...... :$listoptional"
echo "   Compile with debug..... : $msg_debug"
echo "   Compile tests.......... : $msg_tests"
echo "   Man page............... : $msg_man"
echo "   Documentation.......... : $msg_doc"

if test "x$not_asked" != "x" || test "x$not_found" != "x"; then
    echo ""
    echo "Disabled features:"
    if test "x$not_asked" != "x"; then
        echo "   - not asked:$not_asked"
    fi
    if test "x$not_found" != "x"; then
        echo "   - not found:$not_found"
    fi
fi

echo ""
if test "x$WEECHAT_HOME" != "x" ; then
    echo "WeeChat home directory is forced by default to: ${WEECHAT_HOME}"
    echo ""
fi
eval echo "WeeChat will be installed in $bindir"
echo ""
echo "configure complete, now type 'make' to build WeeChat $VERSION"
echo ""
