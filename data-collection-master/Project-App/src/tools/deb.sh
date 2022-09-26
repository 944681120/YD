#!/bin/sh

appname=app-cjq
daemon=app-daemon

echo "当前文件应该在 src/output"
echo  "=========1.读取版本号====================="
echo "`pwd`"
#define VERSION_BUILD_TIME "V01.01.05_220421_15.35.03"
configfile=../VersionConfig.h
line=`cat $configfile`
version=`echo ${line#*\"V}`
if [ -z "$version" ];then
    echo "读取版本号失败"
    exit
fi
binname=$version
version=${version%\"*}
echo "version=$version"
lversion=${version%%_*}
echo "lversion=$lversion"

echo  "=========2.0.create $appname.service======"
cat >$appname.service <<EOF
[Unit]
Description=$appname service

[Service]
ExecStart=/usr/bin/$appname
ExecReload=/bin/kill -HUP $MAINPID
KillMode=process
Restart=always

[Install]
WantedBy=multi-user.target
EOF

echo  "=========2.1.create $daemon.service======"
cat >$daemon.service <<EOF
[Unit]
Description=$daemon service

[Service]
ExecStart=/usr/bin/$daemon
ExecReload=/bin/kill -HUP $MAINPID
KillMode=process
Restart=always

[Install]
WantedBy=multi-user.target
EOF

echo  "=========3..create deb file======"
echo  "all data save in temp .\n"
rm -rf temp
mkdir -p temp
mkdir -p temp/DEBIAN
touch temp/DEBIAN/{control,postinst,postrm}
chmod -R 755 temp/DEBIAN/

cat >temp/DEBIAN/control <<EOF
Package: $appname
Version: $lversion
Architecture: armel
Section: utils
Maintainer: huawei <huawei@huawei.com>
Installed-Size: 2000
Priority: optional
Description: this is app for collection data!
EOF

cat >temp/DEBIAN/postinst <<EOF
#!/bin/bash
if [ ! -d "/$appname" ]; then
	echo "mkdir /$appname"
	mkdir /$appname
fi

if [ ! -d "/$appname/setting" ]; then
	echo "mkdir /$appname/setting"
	mkdir /$appname/setting
fi
if [ ! -d "/$appname/log" ]; then
	echo "mkdir /$appname/log"
	mkdir /$appname/log
fi

if [ ! -d "/$appname/report" ]; then
	echo "mkdir /$appname/report"
	mkdir /$appname/report
fi

cp -rn $appname/tmpinst/setting/*  /$appname/setting/


#===========1. 开机运行=================================================
systemctl enable $appname.service
systemctl enable $daemon.service
# #===========2. 安装deb后运行============================================
# #app
# dpids=\$(ps -ef | grep $appname | grep -v "grep" | awk '{print $2}')
# if [ -z "\$dpids" ];then
#     echo "start service:$appname"
#     systemctl start $appname
# fi

# # sleep 2

# #daemon
# dpids=\$(ps -ef | grep $daemon | grep -v "grep" | awk '{print $2}')
# if [ -z "\$dpids" ];then
#     echo "start service:$daemon"
#     systemctl start $daemon
# fi

rm -rf /$appname/tmpinst

#reboot

EOF

cat > temp/DEBIAN/postrm <<EOF
# rm -rf /$appname
EOF

chmod 0755 temp/DEBIAN -R

mkdir -p temp/lib/systemd/system/
mkdir -p temp/usr/bin/
mkdir -p temp/$appname/
mkdir -p temp/$appname/tmpinst/
mkdir -p temp/$appname/tmpinst/setting/
mkdir -p temp/usr/lib/


# 程序拷贝
mv $appname temp/usr/bin/$appname
mv $daemon  temp/usr/bin/$daemon
# cp database temp/usr/bin/app1srv2
# cp libdatabase.so temp/lib/libdatabase1.so
mv $appname.service temp/lib/systemd/system/$appname.service
mv $daemon.service  temp/lib/systemd/system/$daemon.service

if test "pc" = $1;then
cp ../readme/lib/pc/*  temp/usr/lib/
else
cp ../readme/lib/arm/* temp/usr/lib/
fi

# 临时文件拷贝
cp -rf ../readme/setting/rtu_setting.json temp/$appname/tmpinst/setting/
cp -rf ../readme/setting/zlog.conf        temp/$appname/tmpinst/setting/


chmod a+x temp/usr/bin/$appname
chmod a+x temp/usr/bin/$daemon
chmod 0644 ./temp/lib -R
# chmod -x ./temp/lib/libdatabase1.so
chmod 0755 ./temp/usr -R

dpkg -b temp/ $appname"_V"$version.deb
if [ $? = 0 ]; then
echo "create deb file success!"
else
echo "create deb file failed!"
fi

