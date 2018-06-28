#!/bin/sh

# Check for the following text at the top of each source and header
# file:

# /* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

HGROOT=`hg root`
BASEPATH=`pwd`

# Parse options
OPTIONS=mq
LONGOPTIONS=modified-only,quiet
QUIET=0

PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit 2
fi

eval set -- "$PARSED"

while true; do
	case "$1" in 
		-q|--quiet)
			QUIET=1
			shift
			;;
		-m|--modified-only)
			MODIFIED_ONLY=1
			shift
			;;
		--)
			shift
			break;
			;;
		*)
			echo "Error."
			exit 3
			;;
	esac
done

function eecho()
{
	if [[ $QUIET -eq 0 ]]; then
		echo $*
	fi
}

ERROR=0

# Figure out what files we need to check
FILES=""
if [ $MODIFIED_ONLY ]; then
	FILES=$(hg st -man | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})
else
	FILES=$(hg st -macn | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})
fi

# For each file, grep for the license header (or skip if it should be skipped)
for file in $FILES; do
	SKIPCHECK=`grep -E "^/\* SKIP LICENSE CHECK \*/" $file | head -n1`
	
	if [ "$SKIPGUARDCHECK" != "" ]; then
		continue
	fi
	
	LICENSE="/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */"
	FOUNDLICENSE=$(grep -F "$LICENSE" $file | head -n1)
	
	if [ "$FOUNDLICENSE" = "" ]; then
		LICENSEMISSING=1
	else
		LICENSEMISSING=0
	fi
	
	if [ $LICENSEMISSING -eq 1 ]; then
		ERROR=1
		eecho "License missing in '$file'"
		
		# Don't need to keep looking if we're in quiet mode
		if [ $QUIET -eq 1 ]; then
			break;
		fi;
	fi
	
done

exit $ERROR
