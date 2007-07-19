@echo off

rem Athena のパスを設定する（Readme があるフォルダ）
set ATHENAPATH=..\..\

rem 実行ファイルがあるフォルダ
set BINPATH=%ATHENAPATH%
rem set BINPATH=%ATHENAPATH%bin\

rem 実行ファイル
set L_SRV=%BINPATH%login-server
set C_SRV=%BINPATH%char-server
set M_SRV=%BINPATH%map-server

rem サービス名のサフィックス
rem  同じマシンで複数の Athena サービスを登録する場合に、
rem  設定する必要があります。たとえば 2 などを設定します。
set L_SUF=
set C_SUF=
set M_SUF=

rem サービスの削除
echo サービスを削除しています
%L_SRV% /atnwinsvcuninst /suffix=%L_SUF%
%C_SRV% /atnwinsvcuninst /suffix=%C_SUF%
%M_SRV% /atnwinsvcuninst /suffix=%M_SUF%

echo FAILED が表示されていなければサービスの削除に成功しています。

pause
