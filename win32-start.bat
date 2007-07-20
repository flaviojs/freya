@echo off

IF NOT EXIST .\log\con md .\log

start cmd /k login-server.exe
start cmd /k char-server.exe
start cmd /k map-server.exe
