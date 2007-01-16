@echo off

:restart
char-server.exe
ping -n 10 127.0.0.1 > nul
goto restart