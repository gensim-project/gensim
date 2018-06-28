#!/bin/sh

# Get a known-good version of linux
wget -N https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/snapshot/linux-4.3.tar.gz || exit 1
tar xf linux-4.3.tar.gz || exit 1

# Copy in the known-good configuration
cp realview-config linux-4.3/.config || exit 1

# And build it
ARCH=arm make -C linux-4.3  || exit 1

