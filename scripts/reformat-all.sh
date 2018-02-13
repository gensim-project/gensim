#!/bin/sh

HGROOT=`hg root`
ASTYLE=astyle

echo "Formatting with astyle"

FILES=$(hg st -macn | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})
echo "Reformatting the following files:"
echo $FILES

if [ "$FILES" ]; then
	echo $FILES | xargs $ASTYLE -n --style=linux --indent=tab --indent-switches --indent-namespaces
else
	echo "No files to be reformatted"
fi
