#!/bin/sh

#$1 ： -f,第一个参数.

echo "提示"
echo "./build.sh pc   使用编译环境的gcc"
echo "./build.sh      使用交叉定义的cc"

rm -rf ./build
rm -rf ./output

echo "1.编译程序"
if test "pc" = $1
then
echo "使用编译环境的gcc"
cmake CMakeLists.txt
else
echo "使用交叉定义的cc"
cmake CMakeLists.txt -DCMAKE_TOOLCHAIN_FILE=cross.cmake
fi
make

#删除一些产生的文件
rm -rf CMakeFiles
rm -f cmake_install
rm -f Makefile
rm -f CMakeCache.txt
rm -f cmake_install.cmake

cp ./output/app-daemon* ./output/app-daemon