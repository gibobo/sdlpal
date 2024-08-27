@echo off
del vc17 /q
mkdir vc17
cd vc17
cmake -G "Visual Studio 17 2022" ..
start sdlpal_win32.sln