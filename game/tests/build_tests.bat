@echo off
setlocal EnableDelayedExpansion

REM Test build script for Dystopia MUD
REM Compiles test files and links against game .obj files
REM Requires Visual Studio or Build Tools
REM
REM Usage:
REM   build_tests.bat         - Build and run tests
REM   build_tests.bat clean   - Remove test build artifacts

cd /d "%~dp0"

REM Handle 'clean' argument
if /i "%~1"=="clean" (
    echo Cleaning test artifacts...
    if exist "obj" rmdir /s /q "obj"
    if exist "run_tests.exe" del /q "run_tests.exe"
    echo Clean complete.
    goto :eof
)

REM Locate Visual Studio via vswhere
set VSINSTALL=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath 2^>nul`) do (
    set "VSINSTALL=%%i"
)

if "%VSINSTALL%"=="" (
    echo ERROR: Visual Studio not found. Install Visual Studio or Build Tools.
    exit /b 1
)

REM Set up MSVC environment
if exist "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
) else (
    echo ERROR: vcvarsall.bat not found
    exit /b 1
)

REM Directories
set SRC_DIR=..\src
set GAME_OBJ=..\build\win64\obj\Release
set TEST_OBJ=obj

REM Include paths
set INC=/I. /I%SRC_DIR%\core /I%SRC_DIR%\classes /I%SRC_DIR%\world /I%SRC_DIR%\systems /I%SRC_DIR%\db

REM Compiler flags: match game Release config (/MD /GL) + TEST_BUILD
set CFLAGS=/nologo /W3 /O2 /MD /GL /Zi /DTEST_BUILD /D_CRT_SECURE_NO_WARNINGS /DWIN32 /DHAVE_ZLIB %INC%

REM Create obj directory
if not exist "%TEST_OBJ%" mkdir "%TEST_OBJ%"

echo Building tests...

REM Compile test source files
for %%f in (test_main.c test_helpers.c test_dice.c test_strings.c test_stats.c) do (
    echo   %%f
    cl %CFLAGS% /c %%f /Fo%TEST_OBJ%\ >nul
    if errorlevel 1 (
        echo FAILED to compile %%f
        exit /b 1
    )
)

REM Recompile comm.c with TEST_BUILD to exclude main()
echo   comm.c (TEST_BUILD)
cl %CFLAGS% /c %SRC_DIR%\core\comm.c /Fo%TEST_OBJ%\comm_test.obj >nul
if errorlevel 1 (
    echo FAILED to compile comm.c with TEST_BUILD
    exit /b 1
)

REM Collect all game .obj files EXCEPT comm.obj (flat directory on Windows)
set GAME_OBJS=
for %%f in (%GAME_OBJ%\*.obj) do (
    set "fname=%%~nf"
    if /i not "!fname!"=="comm" (
        set "GAME_OBJS=!GAME_OBJS! %%f"
    )
)

REM Libraries
set LIBS=ws2_32.lib bcrypt.lib ..\lib\win64\Release\zlib.lib

REM Link
echo Linking...
link /nologo /LTCG /DEBUG /OUT:run_tests.exe %TEST_OBJ%\test_main.obj %TEST_OBJ%\test_helpers.obj %TEST_OBJ%\test_dice.obj %TEST_OBJ%\test_strings.obj %TEST_OBJ%\test_stats.obj %TEST_OBJ%\comm_test.obj %GAME_OBJS% %LIBS%
if errorlevel 1 (
    echo LINK FAILED
    exit /b 1
)

echo.
echo Build successful. Running tests...
echo ========================================
"%~dp0run_tests.exe"
set RESULT=%ERRORLEVEL%
echo ========================================

if %RESULT% equ 0 (
    echo All tests passed!
) else (
    echo Some tests FAILED.
)

endlocal
exit /b %RESULT%
