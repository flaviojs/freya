@echo off

:restart
echo %date% - %time% : char-server started	>> log/launcher.log
char-server.exe
echo %date% - %time% : char-server crashed	>> log/launcher.log
ping -n 10 127.0.0.1 > nul
goto restart