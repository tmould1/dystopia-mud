@echo off
setlocal

REM MinGW build script for Dystopia MUD
REM Requires MSYS2 with MinGW-w64 installed at C:\msys64
REM Run from project root or build directory

set MSYS_PATH=C:\msys64\usr\bin
set MINGW_PATH=C:\msys64\mingw64\bin
set PATH=%MINGW_PATH%;%MSYS_PATH%;%PATH%

REM Navigate to project root (parent of build directory)
cd /d "%~dp0.."

REM Create logs directory if it doesn't exist
if not exist "build\logs" mkdir "build\logs"

REM Generate timestamp for log file
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set LOGFILE=build\logs\build_%datetime:~0,8%-%datetime:~8,6%.log

REM Run make and log output
echo Building Dystopia MUD... (logging to %LOGFILE%)
make -f build/Makefile.mingw %* 2>&1 | C:\msys64\usr\bin\tee.exe "%LOGFILE%"

echo.
echo Build log saved to: %LOGFILE%
