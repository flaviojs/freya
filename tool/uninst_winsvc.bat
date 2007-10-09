@echo off

rem 服务端路径
set L_SRV=login-server
set C_SRV=char-server
set M_SRV=map-server

rem 注册的服务名
rem 在一台机器上开2个或以上的Athena时的设定。
set L_SUF=
set C_SUF=
set M_SUF=

rem 卸载服务
echo 卸载服务
%L_SRV% /atnwinsvcuninst /suffix=%L_SUF%
%C_SRV% /atnwinsvcuninst /suffix=%C_SUF%
%M_SRV% /atnwinsvcuninst /suffix=%M_SUF%

echo 如果没有显示 FAILED 的话，表示卸载服务成功。

pause
