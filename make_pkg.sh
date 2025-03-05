#!/bin/sh
make -f Makefile.gnu
make -f Makefile.gnu install DESTDIR=`pwd`/smokerand_1.0-1
dpkg-deb --build smokerand_1.0-1