#!/bin/sh

# Check for the following text at the top of each source and header
# file:

# /* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

FILES=`find . -regextype egrep -regex ".*/.*\.(h|cpp|S)"`
BASEPATH=`pwd`

ERROR=0

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
		echo "License missing in '$file'"
	fi
	
done

exit $ERROR
