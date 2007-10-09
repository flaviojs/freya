@echo off

rem 记录Athena日志的文件夹
set LOGPATH=./log/

rem 服务端路径
set L_SRV=login-server
set C_SRV=char-server
set M_SRV=map-server

rem conf文件设定
set L_SRV_C=./conf/login_athena.conf
set C_SRV_C=./conf/char_athena.conf
set C_SRV_C2=./conf/inter_athena.conf
set M_SRV_C=./conf/map_athena.conf
set M_SRV_C2=./conf/battle_athena.conf
set M_SRV_C3=./conf/atcommand_athena.conf
set M_SRV_C4=./conf/script_athena.conf
set M_SRV_C5=./conf/msg_athena.conf
set M_SRV_C6=./conf/grf-files.txt

rem 注册的服务名
rem 在一台机器上开2个或以上的Athena时的设定。
set L_SUF=
set C_SUF=
set M_SUF=

rem 服务器常规日志路径
rem 在服务器与服务器之间不可以指定同样的文件名。
rem 如果是一台机器开多个Athena时请特别要注意。
set L_OUT=%LOGPATH%login_svc_stdout.log
set C_OUT=%LOGPATH%char_svc_stdout.log
set M_OUT=%LOGPATH%map_svc_stdout.log

rem 服务器错误日志路径
rem 不能指定与常规日志相同的文件名。
rem 对于错误日志来说，几乎没有输出，所以不指定也无所谓。
set L_ERR=
set C_ERR=
set M_ERR=
rem set L_ERR=%LOGPATH%login_svc_stderr.log
rem set C_ERR=%LOGPATH%char_svc_stderr.log
rem set M_ERR=%LOGPATH%map_svc_stderr.log

rem 安装服务
echo 安装服务
%L_SRV% /atnwinsvcinst /suffix=%L_SUF% /stdout=%L_OUT% /stderr=%L_ERR% %L_SRV_C%
%C_SRV% /atnwinsvcinst /suffix=%C_SUF% /stdout=%C_OUT% /stderr=%C_ERR% %C_SRV_C% %C_SRV_C2%
%M_SRV% /atnwinsvcinst /suffix=%M_SUF% /stdout=%M_OUT% /stderr=%M_ERR% %M_SRV_C% %M_SRV_C2% %M_SRV_C3% %M_SRV_C4% %M_SRV_C5% %M_SRV_C6%

echo 如果没有显示 FAILED 的话，表示安装服务成功。
echo.
echo 根据doc/windows_service.txt，
echo 进行好余下的设定后，就请启动服务器吧。

pause
