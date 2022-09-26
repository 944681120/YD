#!/bin/bash

rm -rf temp
mkdir -p temp
mkdir -p temp/DEBIAN
touch temp/DEBIAN/{control,postinst,postrm}
chmod -R 755 temp/DEBIAN/

echo "Package: app-cjq" > temp/DEBIAN/control
echo "Version: 1.001" >> temp/DEBIAN/control
echo "Architecture: armel" >> temp/DEBIAN/control
echo "Section: utils" >> temp/DEBIAN/control
echo "Maintainer: huawei <huawei@huawei.com>" >> temp/DEBIAN/control
echo "Installed-Size: 2000" >> temp/DEBIAN/control
echo "Priority: optional" >> temp/DEBIAN/control
echo "Description: this is app-cjq" >> temp/DEBIAN/control

echo "#!/bin/bash" > temp/DEBIAN/postinst
echo "mkdir -p /app-cjq" >> temp/DEBIAN/postinst

echo "#!/bin/bash" > temp/DEBIAN/postrm
echo "rm -rf /app-cjq" >> temp/DEBIAN/postrm

mkdir -p temp/lib/systemd/system/
mkdir -p temp/usr/bin/
mkdir -p temp/lib/

cp database temp/usr/bin/app-cjq
#cp database temp/usr/bin/app1srv2
cp libdatabase.so temp/lib/libdatabase1.so
cp app-cjq.service temp/lib/systemd/system/
#cp app1srv2.service temp/lib/systemd/system/

chmod a+x temp/usr/bin/app-cjq
#chmod a+x temp/usr/bin/app1srv2
chmod 0644 ./temp/lib -R
chmod -x ./temp/lib/libdatabase1.so
chmod 0755 ./temp/usr -R

dpkg -b temp/ app-cjq.deb
if [ $? = 0 ]; then
echo "create deb file success!"
exit 0
else
echo "create deb file failed!"
exit 0
fi