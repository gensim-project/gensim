#!/bin/sh

HGROOT=`hg root`
BASEPATH=`pwd`
FILES=$(hg st -macn | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})

REGEX="//.*HACK:"

FOUND=0

for file in $FILES; do
	grep -E "$REGEX" $file -q
	if [ $? -eq 0 ]; then
		FOUND=1
	fi
done

if [ $FOUND -eq 1 ]; then
	echo "Detected some comments that might indicate hacks. Please fix before committing!"
	exit 1
fi
