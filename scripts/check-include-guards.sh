#!/bin/sh

# This script checks header files (.h files) for the presence of 
# include guards, and complains if the include guards are not in a 
# particular format (_PATH_TO_HEADER_HEADER_NAME_H_)

FILES=`find . -regextype sed -regex ".*/.*\.h"`
BASEPATH=`pwd`

ERROR=0

for file in $FILES; do
	SKIPGUARDCHECK=`grep -E "^/\* SKIP GUARD CHECK \*/" $file | head -n1`
	
	if [ "$SKIPGUARDCHECK" != "" ]; then
		continue
	fi
	
	GUARD="_`echo ${file^^} | sed 's/^\.\///g ; s/\./_/g ; s/\//_/g' -`_"

	IFNDEF=`grep -E "^#ifndef $GUARD" $file | head -n1`
	DEFINE=`grep -E "^#ifndef $GUARD" $file | head -n1`
	ENDIF=`grep -E "^#endif /\* $GUARD \*/" $file | head -n1`
	
	if [ "$IFNDEF" = "" ]; then
		IFNDEFMISSING=1
	else
		IFNDEFMISSING=0
	fi
	
	if [ "$DEFINE" = "" ]; then
		DEFINEMISSING=1
	else
		DEFINEMISSING=0
	fi
	
	if [ "$ENDIF" = "" ]; then
		ENDIFMISSING=1
	else
		ENDIFMISSING=0
	fi
	
	if [ $IFNDEFMISSING -eq 1 -o $DEFINEMISSING -eq 1 -o $ENDIFMISSING -eq 1 ]; then
		echo -n "$file ($GUARD): "
		
		if [ $IFNDEFMISSING -eq 1 ]; then
			echo -n "IFNDEF missing! "
		fi
		
		if [ $DEFINEMISSING -eq 1 ]; then
			echo -n "DEFINE missing! "
		fi
		
		if [ $ENDIFMISSING -eq 1 ]; then
			echo -n "ENDIF missing!"
		fi
		
		echo ""
		
		ERROR=1
	fi
done

exit $ERROR
