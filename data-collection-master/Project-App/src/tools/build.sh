#!/bin/sh

#$1 ： -f,第一个参数.

echo "提示"
echo "./build.sh pc   使用编译环境的gcc"
echo "./build.sh      使用交叉定义的cc"

cd ../

rm -rf ./build
rm -rf ./output

echo "=====================1.编译程序==========================="
if test "pc" = $1
then
echo "使用编译环境的gcc"
cmake CMakeLists.txt  -DBUILDUSING_GTEST=OFF
echo "pc zlog 拷贝"
cp  ./readme/lib/pc/*  /usr/lib/
else
echo "使用交叉定义的cc"
cmake CMakeLists.txt -DCMAKE_TOOLCHAIN_FILE=cross.cmake -DBUILDUSING_GTEST=OFF
echo "arm zlog 拷贝"
cp  ./readme/lib/arm/*  /usr/lib/
fi
make

#删除一些产生的文件
rm -rf CMakeFiles
rm cmake_install
rm Makefile
rm CMakeCache.txt
rm cmake_install.cmake


#测试
echo "=====================2.测试程序版本==========================="
cd output
for name in `ls app-cjq*`
do
    echo "应用程序为:"$name
done

for version in `./app-cjq* --version`
do
    echo "版本为:"$version
done
cd ../

# echo "=====================3.编译app-daemon===================="
cd ./daemon
if test "pc" = $1
then
./build.sh pc
else
./build.sh
fi
pwd
cp ./output/app-daemon  ../output/app-daemon
cd ../output/


# #setting 的文件
echo "=====================3.拷贝文件==========================="
# cp -rf ../readme/setting ./
# echo "哭文件"
# mkdir lib

echo "=====================4.打包==========================="
if test "pc" = $1;then
../tools/deb.sh pc
else
../tools/deb.sh
fi



# echo "=====================5.md5打包==========================="
# md5sum ./app-cjq*  >app-cjq.md5
# tar -cf $name.tar $name.deb app-cjq.md5
# ls $name.tar
