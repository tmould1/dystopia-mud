@echo off
setlocal

REM MSVC build script for Dystopia MUD
REM Requires Visual Studio or Build Tools with MSBuild
REM Run from game/build directory
REM
REM Usage:
REM   build.bat              - Build Release configuration
REM   build.bat debug        - Build Debug configuration
REM   build.bat clean        - Remove build artifacts

REM Stay in build directory (where this script lives)
cd /d "%~dp0"

REM Handle 'clean' argument
if /i "%~1"=="clean" (
    echo Cleaning build artifacts...
    if exist "win64\obj" rmdir /s /q "win64\obj"
    if exist "..\..\gamedata\dystopia.exe" del /q "..\..\gamedata\dystopia.exe"
    if exist "..\..\gamedata\dystopia_new.exe" del /q "..\..\gamedata\dystopia_new.exe"
    echo Clean complete.
    goto :eof
)

REM Select configuration (default: Release)
set CONFIG=Release
if /i "%~1"=="debug" set CONFIG=Debug

REM Locate MSBuild via vswhere (works with VS 2017+)
set MSBUILD=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do (
    set "MSBUILD=%%i"
)

if "%MSBUILD%"=="" (
    echo ERROR: MSBuild not found. Install Visual Studio or Build Tools.
    echo        https://visualstudio.microsoft.com/downloads/
    exit /b 1
)

REM Create logs directory if it doesn't exist
if not exist "logs" mkdir "logs"

REM Create gamedata directory if it doesn't exist (output location)
if not exist "..\..\gamedata" mkdir "..\..\gamedata"

REM Detect number of CPU cores for parallel build
set JOBS=4
for /f "tokens=*" %%i in ('echo %NUMBER_OF_PROCESSORS%') do set JOBS=%%i

REM Generate timestamp for log file
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value 2^>nul') do set datetime=%%I
set LOGFILE=logs\build_%datetime:~0,8%-%datetime:~8,6%.log

REM Run MSBuild with parallel compilation and log output
echo Building Dystopia MUD (%CONFIG%) with %JOBS% parallel jobs... (logging to %LOGFILE%)
"%MSBUILD%" dystopia.vcxproj /p:Configuration=%CONFIG% /p:Platform=x64 /m:%JOBS% /v:minimal /fl /flp:logfile=%LOGFILE%;verbosity=normal

if errorlevel 1 (
    echo.
    echo BUILD FAILED - see %LOGFILE% for details
    exit /b 1
)

echo.
echo Build successful.
echo Build log saved to: %LOGFILE%

endlocal
