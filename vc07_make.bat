@echo off
rem VC++ 6.0 / VC++ .net / VC++ .net 2003 / VC++ .net 2005 / VC++ ToolKit 2003 
rem ビルド用バッチファイル

rem ---------------------------
rem パスの設定

rem VC++ Toolkit
Set PATH=C:\Program Files\Microsoft Visual C++ Toolkit 2003\bin;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
Set INCLUDE=C:\Program Files\Microsoft Visual C++ Toolkit 2003\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
Set LIB=C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%

rem VC++ .net 2005
rem Set PATH=C:\Program Files\Microsoft Visual Studio 8\VC\bin;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem Set INCLUDE=C:\Program Files\Microsoft Visual Studio 8\VC\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem Set LIB=C:\Program Files\Microsoft Visual Studio 8\VC\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%

rem VC++ .net 2005 / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\VCVARS32.BAT"

rem VC++ .net 2003 / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"

rem VC++ .net (2002) / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio .NET\Vc7\bin\vcvars32.bat"

rem VC++ 6.0 / 必要ならコメントアウトをはずす
rem ここ以外に __opt2__ の /MAP と /nologo を消す必要がある
rem call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"


rem MySQLのIncludeとlibmysql.libの保存場所　環境毎に書き換える事
rem SQL版にする場合下記２行のコメントアウトをはずして下さい。
rem set __sqlinclude__=-I../sql/include/
rem set __sqllib__=../sql/vclib/libmysql.lib

rem -------------------------------------------------------------
rem ビルドオプションの選択

rem txt/sql 選択 ： sql にするならコメントアウトする
set __TXT_MODE__=/D "TXT_ONLY"

rem txt で、ジャーナルを使うならコメントアウトをはずす
rem set __TXT_MODE__=/D "TXT_ONLY" /D "TXT_JOURNAL"

rem login_id2 や IP でごにょごにょしたい人はコメントアウトをはずす
rem set __CMP_AFL2__=/D "CMP_AUTHFIFO_LOGIN2"
rem set __CMP_AFIP__=/D "CMP_AUTHFIFO_IP"

rem httpd を完全に無効にする場合コメントアウトをはずす
rem set __NO_HTTPD__=/D "NO_HTTPD"

rem httpd で外部 CGI を使う場合はコメントアウトする
set __NO_HTTPD_CGI__=/D "NO_HTTPD_CGI"

rem csvdb を完全に無効にする場合コメントアウトをはずす
rem set __NO_CSVDB__=/D "NO_CSVDB"

rem csvdb のスクリプトからの利用を無効にする場合コメントアウトをはずす
rem set __NO_CSVDB_SCRIPT__=/D "NO_CSVDB_SCRIPT"

rem TK SG SL でごにょごにょしたい人はコメントアウトをはずす
rem set __EXCLASS__=/D "TKSGSL"
rem set __EXCLASS__=/D "TKSGSLGSNJ"

rem 動的にMOBのsc_dataを確保したい人はコメントアウトをはずす
rem set __DYNAMIC_STATUS_CHANGE__=/D "DYNAMIC_SC_DATA"

rem account regist MailAddress
rem set __AC_MAIL__=/D "AC_MAIL"

rem ---------------------------
rem コンパイルオプション設定

@rem CPU最適化スイッチ(By Nameless)
@rem 以下の例を参考にスイッチ名を記入してください。
set _model_=x32

@rem CPUアーキテクチャ32BitCPU/64BitCPU
if "%_model_%"=="x32" set __cpu__=/c /W3 /O2 /Op /GA /TC /Zi
if "%_model_%"=="x64" set __cpu__=/c /arch:SSE2 /W3 /O2 /Op /GA /TC /Zi

@rem メモリー1024以上搭載の32bitCPU/64bitCPU
if "%_model_%"=="HiMemL" set __cpu__=/c /bigobj /W3 /O2 /Op /GA /TC /Zi
if "%_model_%"=="HiMemH" set __cpu__=/c /bigobj /arch:SSE2 /W3 /O2 /Op /GA /TC /Zi

@rem スタック制御をコンパイラで行う場合
if "%_model_%"=="Stac32" set __cpu__=/c /F4096 /W3 /O2 /Op /GA /TC /Zi
if "%_model_%"=="Stac64" set __cpu__=/c /F4096 /arch:SSE2 /W3 /O2 /Op /GA /TC /Zi
@rem AMD系64bitCPU用
if "%_model_%"=="A64x2" set __cpu__=/c /favor:blend /W3 /O2 /Op /GA /TC /Zi
if "%_model_%"=="A64x1" set __cpu__=/c /favor:AMD64 /W3 /O2 /Op /GA /TC /Zi

@rem Intel系64bitCPU用
if "%_model_%"=="EM64T" set __cpu__=/c /favor:EM64T /W3 /O2 /Op /GA /TC /Zi

@rem 以下実験段階(人柱求む by Nameless)
@rem 暴走風味…32BitCPU最高速モード
if "%_model_%"=="mode01" set __cpu__=/c /fp:fast /F4096 /bigobj /W3 /Ox /GA /TC /Zi
@rem 暴走風味…64BitCPU最高速モード
if "%_model_%"=="mode02" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /W3 /Ox /Gr /GA /TC /Zi
@rem 暴走風味…AMD 64x2 & FX系最適化・最高速
if "%_model_%"=="mode03" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:AMD64 /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode04" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:blend /W3 /Ox /Gr /GA /TC /Zi
@rem 暴走風味…Intel 64bitCPU用最適化・最高速
if "%_model_%"=="mode05" set __cpu__=/c /arch:SSE2 /fp:fast /F4096 /bigobj /favor:EM64T /W3 /Ox /Gr /GA /TC /Zi
@rem 以下リザーブ
if "%_model_%"=="mode06" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode07" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode08" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi
if "%_model_%"=="mode09" set __cpu__=/c /W3 /Ox /Gr /GA /TC /Zi

set __opt1__=/I "../common/zlib/" /I "../common/" /D "PACKETVER=7" /D "NEW_006b" /D "FD_SETSIZE=4096"  /D "LOCALZLIB" /D "NDEBUG" /D "_CONSOLE" /D "WIN32" /D "_WIN32" /D "_WIN32_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" %__TXT_MODE__% %__CMP_AFL2__% %__CMP_AFIP__% %__NO_HTTPD__% %__NO_HTTPD_CGI__% %__NO_CSVDB__% %__NO_CSVDB_SCRIPT__% %__EXCLASS__% %__DYNAMIC_STATUS_CHANGE__% %__AC_MAIL__%

set __opt2__=/DEBUG /MAP /nologo user32.lib ../common/zlib/*.obj ../common/*.obj *.obj

rem ---------------------------
rem コンパイル
@echo zlibコンパイル
cd src\common\zlib
cl %__cpu__% %__opt1__% *.c
cd ..\
cl %__cpu__% %__opt1__% *.c 

@echo ログインサーバーコンパイル
cd ..\login
cl %__cpu__% %__opt1__% %__sqlinclude__% *.c
link %__opt2__% %__sqllib__% /out:"../../bin/login-server.exe"

@echo キャラクターサーバーコンパイル
cd ..\char
cl %__cpu__% %__opt1__% %__sqlinclude__% *.c
link %__opt2__% %__sqllib__% /out:"../../bin/char-server.exe"

@echo マップサーバーコンパイル
cd ..\map
cl %__cpu__% %__opt1__% *.c
link %__opt2__% /out:"../../bin/map-server.exe"

@echo コンバーターコンパイル
cd ..\converter
cl %__cpu__% %__opt1__% %__sqlinclude__% *.c
link %__opt2__% %__sqllib__% /out:"../../bin/txt-converter.exe"

cd ..\..\

@echo オブジェクトファイル等のクリーンアップ
del src\common\zlib\*.obj
del src\common\*.obj
del src\char\*.obj
del src\login\*.obj
del src\map\*.obj
del src\converter\*.obj

pause