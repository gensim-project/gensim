#!/bin/sh

# Script to perform precommit checks on the repository (formatting and
# license headers).
#
# To install this hook, run
#
# hg config --local
#
# And add the lines:
# [hooks]
# precommit = sh scripts/precommit-hooks.sh
#

ROOT=`hg root`

ERROR=0

sh scripts/reformat.sh --dry-run --quiet --modified-only

if [[ $? -eq 1 ]]; then
	echo "Formatting error. Run scripts/reformat.sh to correct."
	ERROR=1
fi

sh scripts/check-license.sh --quiet --modified-only

if [[ $? -eq 1 ]]; then
	echo "License header error. Run scripts/check-license.sh to see affected files."
	ERROR=1
fi

exit $ERROR
