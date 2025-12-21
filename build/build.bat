@echo off
setlocal

REM MinGW build script for Dystopia MUD
REM Requires MSYS2 with MinGW-w64 installed at C:\msys64
REM Run from project root or build directory
REM
REM Usage:
REM   build.bat          - Build the project (parallel compilation)
REM   build.bat clean    - Remove object files and executables
REM   build.bat install  - Build dystopia.exe (fresh install)

set MSYS_PATH=C:\msys64\usr\bin
set MINGW_PATH=C:\msys64\mingw64\bin
set PATH=%MINGW_PATH%;%MSYS_PATH%;%PATH%

REM Navigate to project root (parent of build directory)
cd /d "%~dp0.."

REM Handle 'clean' argument - no logging needed
if /i "%~1"=="clean" (
    echo Cleaning build artifacts...
    make -f build/Makefile.mingw clean
    echo Clean complete.
    goto :eof
)

REM Create logs directory if it doesn't exist
if not exist "build\logs" mkdir "build\logs"

REM Create obj directory if it doesn't exist
if not exist "build\obj" mkdir "build\obj"

REM Detect number of CPU cores for parallel build
set JOBS=4
for /f "tokens=*" %%i in ('echo %NUMBER_OF_PROCESSORS%') do set JOBS=%%i

REM Generate timestamp for log file
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set LOGFILE=build\logs\build_%datetime:~0,8%-%datetime:~8,6%.log

REM Run make with parallel jobs and log output
echo Building Dystopia MUD with %JOBS% parallel jobs... (logging to %LOGFILE%)
make -j%JOBS% -f build/Makefile.mingw %* 2>&1 | C:\msys64\usr\bin\tee.exe "%LOGFILE%"

echo.
echo Build log saved to: %LOGFILE%
