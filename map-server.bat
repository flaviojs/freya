@echo off

:restart
echo %date% - %time% : map-server started	>> log/launcher.log
map-server.exe
echo %date% - %time% : map-server crashed	>> log/launcher.log
ping -n 10 127.0.0.1 > nul
goto restart