@echo off

rem bat ファイルから見た Athena のフォルダ（Readme があるフォルダ）
set ATHENAPATH=..\..\

rem bat ファイルから見た、実行ファイルがあるフォルダ
set BINPATH=%ATHENAPATH%
rem set BINPATH=%ATHENAPATH%bin\

rem Athena フォルダから見た、ログファイルのフォルダ
set LOGPATH=./log/

rem 実行ファイル
set L_SRV=%BINPATH%login-server
set C_SRV=%BINPATH%char-server
set M_SRV=%BINPATH%map-server

rem コンフィグファイル
set L_SRV_C=./conf/login_athena.conf
set C_SRV_C=./conf/char_athena.conf
set C_SRV_C2=./conf/inter_athena.conf
set M_SRV_C=./conf/map_athena.conf
set M_SRV_C2=./conf/battle_athena.conf
set M_SRV_C3=./conf/atcommand_athena.conf
set M_SRV_C4=./conf/script_athena.conf
set M_SRV_C5=./conf/msg_athena.conf
set M_SRV_C6=./conf/grf-files.txt

rem サービス名のサフィックス
rem  同じマシンで複数の Athena サービスを登録する場合に、
rem  設定する必要があります。たとえば 2 などを設定します。
set L_SUF=
set C_SUF=
set M_SUF=

rem 標準出力のログファイル
rem  サーバー間で同じファイルは指定できません。
rem  同じマシンで複数のサービスなどを登録する場合は特に注意してください。
set L_OUT=%LOGPATH%login_svc_stdout.log
set C_OUT=%LOGPATH%char_svc_stdout.log
set M_OUT=%LOGPATH%map_svc_stdout.log

rem 標準エラー出力のログファイル
rem  標準出力と同じファイルは指定できません。
rem  標準エラーにはほとんどデータは出力されないので、未指定でもかまいません。
set L_ERR=
set C_ERR=
set M_ERR=
rem set L_ERR=%LOGPATH%login_svc_stderr.log
rem set C_ERR=%LOGPATH%char_svc_stderr.log
rem set M_ERR=%LOGPATH%map_svc_stderr.log

rem サービスの登録
echo サービスを登録しています
%L_SRV% /atnwinsvcinst /suffix=%L_SUF% /stdout=%L_OUT% /stderr=%L_ERR% %L_SRV_C%
%C_SRV% /atnwinsvcinst /suffix=%C_SUF% /stdout=%C_OUT% /stderr=%C_ERR% %C_SRV_C% %C_SRV_C2%
%M_SRV% /atnwinsvcinst /suffix=%M_SUF% /stdout=%M_OUT% /stderr=%M_ERR% %M_SRV_C% %M_SRV_C2% %M_SRV_C3% %M_SRV_C4% %M_SRV_C5% %M_SRV_C6%

echo FAILED が表示されていなければサービスの登録に成功しています。
echo.
echo doc/windows_service.txt にしたがって、
echo 残りの設定を行ったあとにサービス開始してください。

pause
