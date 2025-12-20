@echo off
setlocal

REM MinGW build script for Dystopia MUD
REM Requires MSYS2 with MinGW-w64 installed at C:\msys64

set MSYS_PATH=C:\msys64\usr\bin
set MINGW_PATH=C:\msys64\mingw64\bin
set PATH=%MINGW_PATH%;%MSYS_PATH%;%PATH%

cd /d "%~dp0"
make -f Makefile.mingw %*
