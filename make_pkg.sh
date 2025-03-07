#!/bin/sh
make -f Makefile.gnu
rm -rf smokerand_1.0-1/usr
make -f Makefile.gnu install DESTDIR=`pwd`/smokerand_1.0-1
dpkg-deb --build smokerand_1.0-1
