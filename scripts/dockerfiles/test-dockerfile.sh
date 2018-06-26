#!/bin/sh

DOCKERFILEDIR=$1
BUILDDIR="build-$(dirname $DOCKERFILEDIR/asdf)"

echo "Running with dockerfile in $1"

IMAGEID=$(cd $DOCKERFILEDIR && docker build . -q)

if [ "$?" -ne 0 ]; then
	exit 1
fi

echo "Got image $IMAGEID"

echo "Testing build..."

docker run -u $(id -u) --rm -v $(hg root):/workspace $IMAGEID bash -c \
	 "\
	 cd /workspace && \
	 mkdir -p $BUILDDIR && \
	 cd $BUILDDIR && \
	 cmake .. && \
	 make \
	 "

SUCCESS=$?
echo "Finished with $SUCCESS"

if [ "$SUCCESS" -eq 0 ]; then
	echo "Success!"
	exit 0
else
	echo "FAILURE"
	exit 1
fi
