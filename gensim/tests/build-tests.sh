#!/bin/bash

# Echo a message to stderr
function reverb () {
	(>&2 echo "$*" )
}

# Look at the given makefile and extract the v///alue of a variable
# $1 = Variable to print
# $2 = Path to Makefile
function get_make_var() {
    echo 'unique123:;@echo ${'"$1"'}' | 
      make -f - -f "$2" --no-print-directory unique123
}

# recursively search for sources to add
# $1 = The path to the makefile to add
function find_sources() {
	local LFILES=""
	
	reverb "Examining $1"
	
	if [ ! -f $1 ]; then
		reverb "Could not find Makefile $1!"
		echo "";
		return
	fi
	
	# recurse into all subdirectories in SUBDIRECTORIES
	local SUBDIRS=`get_make_var SUBDIRECTORIES $1`
	local BASEDIR=`dirname $1`
	for SUBDIR in $SUBDIRS; do 
		LFILES="$LFILES `find_sources $BASEDIR/$SUBDIR/Makefile`"
	done
	
	# add all sources in SOURCES
	local LOCALSOURCES=`get_make_var SOURCES $1`
	for i in $LOCALSOURCES; do
		local LFILE="$BASEDIR/$i"
		LFILES="$LFILES $LFILE"
	done
	
	# return discovered files
	echo $LFILES
}

make gtest/libgtest.a

SOURCES=`find_sources Makefile`

if [ "$SOURCES" = "" ]; then
	reverb "No sources found!"
	exit 1
fi

echo "Found these sources:"
for i in $SOURCES; do
	echo " - $i"
done

# look for antlr
ANTLRPATH=`get_make_var ANTLR_PATH ../Makefile.config`
reverb "Looking for ANTLR at $ANTLRPATH"

CXX=g++
CFLAGS="-Igtest/googletest/include/ -I`pwd`/../inc/ -I`pwd` -I$ANTLRPATH/include/ -g"
LIBS="-lpthread -L../dist/lib/ -lgensim -Wl,--no-whole-archive `pwd`/../dist/lib/libgensim_grammar.a -L$ANTLRPATH/lib -lantlr3c gtest/libgtest.a gtest/libgtest_main.a"
${CXX} ${CFLAGS} ${SOURCES} ${LIBS}	-o all-tests
