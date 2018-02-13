#!/bin/sh

SUBREPO_CHANGED=`( hg id | grep -q "\+" ); echo $?`

if [ "$SUBREPO_CHANGED" = "0" ]; then
    echo "Creating pass commit"
    hg ci -m "Integration tests passed" -u "Jenkins" && hg push
else
    echo "No changes detected"
fi

exit 0
