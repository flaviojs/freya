@echo off

echo > log/launcher.log

start cmd /k login-server.bat
start cmd /k char-server.bat
start cmd /k map-server.bat