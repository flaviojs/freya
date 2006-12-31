@echo off

:restart
echo %date% - %time% : login-server started	>> log/launcher.log
login-server.exe
echo %date% - %time% : login-server crashed	>> log/launcher.log
ping -n 10 127.0.0.1 > nul
goto restart