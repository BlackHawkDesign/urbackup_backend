#!/bin/sh

set -e

if ! test -e build_client_ok
then
	./switch_build.sh client
	autoreconf || true
	automake --add-missing || true
	libtoolize || true
	autoreconf --install
	./configure
	touch build_client_ok
fi
./switch_build.sh client
make
make dist