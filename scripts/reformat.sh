#!/bin/sh

HGROOT=`hg root`
ASTYLE=astyle

OPTIONS=dmq
LONGOPTIONS=dry-run,modified-only,quiet

PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit 2
fi

eval set -- "$PARSED"

QUIET=0
MODIFIED_ONLY=0
DRY_RUN=0

function eecho()
{
	if [[ $QUIET -eq 0 ]]; then
		echo $*
	fi
}

while true; do
	case "$1" in 
		-d|--dry-run)
			DRY_RUN=1
			shift
			;;
		-m|--modified-only)
			MODIFIED_ONLY=1
			shift
			;;
		-q|--quiet)
			QUIET=1
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

eecho "Formatting with astyle"

FILES=""

if [[ $MODIFIED_ONLY -eq 1 ]]; then
	FILES=$(hg st -man | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})
else
	FILES=$(hg st -macn | grep -E "\.(cpp|h)$" | xargs -i echo $HGROOT/{})
fi

if [ "$FILES" ]; then
	if [ $DRY_RUN -eq 1 ]; then
		! echo $FILES | xargs $ASTYLE -n --style=linux --indent=tab --indent-switches --indent-namespaces --dry-run | grep -E "^Formatted" > /dev/null
		exit $?
	else
		echo $FILES | xargs $ASTYLE -n --style=linux --indent=tab --indent-switches --indent-namespaces $ASTYLE_OPTS
	fi
else
	eecho "No files to be reformatted"
fi
