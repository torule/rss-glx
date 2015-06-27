#!/bin/sh
set -e

libtoolize --force --copy
aclocal
autoheader
automake -a --copy
autoconf

