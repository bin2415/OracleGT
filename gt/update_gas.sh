#!/bin/bash
echo "rm -r build_arm/binutils-2.30/gas"
rm -r build_arm/binutils-2.30/gas
echo "cp -r binutils/gas-2.30/ build_arm/binutils-2.30/gas"
cp -r binutils/gas-2.30/ build_arm/binutils-2.30/gas
echo "cd build_arm/binutils-2.30/build"
cd build_arm/binutils-2.30/build
echo "-----------------build gas------------------------"
make install -j | grep "error"
cd ../../executable_binutils/bin
./as /opt/shared/test.s 2>&1 | tee /tmp/tmp.log
