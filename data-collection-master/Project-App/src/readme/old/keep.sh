#!/bin/sh
app=app-cjq

while true; do
        #启动一个循环，定时检查进程是否存在
        n=`ps -aux | grep $app | grep -v grep`
        echo "$app=[$n]"
        if test "" = "$n" ; then
            echo "restart $app"
            #如果不存在就重新启动
            service $app restart
            #启动后沉睡10s
            sleep 10
        else
            echo "$app=$n"
        fi
        #每次循环沉睡10s
        sleep 5
    done
