#!/bin/sh

###
### common stuff
###
OK="\\033[70G[\\033[1;32mOK\\033[1;00m]"
FAIL="\\033[70G[\\033[1;31mFAILED\\033[1;00m]"

AUTOGEN_LOG=autogen.log

err ()
{
    echo "-------"
    echo "Error :"
    echo "---8<-----------------------------------"
    cat $AUTOGEN_LOG
    echo "----------------------------------->8---"
    exit 1
}

run ()
{
    echo -n "Running \"$@\""
    eval $@ >$AUTOGEN_LOG 2>&1
    if [ $? = 0 ] ; then
	echo -e $OK
    else
	echo -e $FAIL
	err
    fi
}

###
### cleanning part
###
#  remove autotools stuff
run "rm -rf config"
run "rm -f config.h.in"
run "rm -f aclocal.m4 configure config.log config.status"
run "rm -rf autom4te*.cache"
# remove libtool stuff
run "rm -f libtool"
# remove gettext stuff
run "rm -f ABOUT-NLS"
run "rm -rf intl"

###
### configuration part
###
# create the config directory
run "mkdir -p config/m4"
run "mkdir intl"

# execute autotools cmds
run "autopoint -f"
run "libtoolize --automake --force --copy"
run "aclocal --force -I config/m4"
run "autoheader"
run "autoconf"
run "automake --add-missing --copy --gnu"

# ending
rm -f $AUTOGEN_LOG
