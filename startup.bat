@echo off
REM Written by Furey.
REM With additions from Tony.
REM Converted to batch by Claude.

setlocal enabledelayedexpansion

REM Get the directory where this script lives
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM Load port from server.conf if available (default 8888)
set port=8888
if exist "%SCRIPT_DIR%\..\server.conf" (
    for /f "tokens=1,* delims==" %%a in ('findstr /b "PORT=" "%SCRIPT_DIR%\..\server.conf"') do set port=%%b
)
if exist "%SCRIPT_DIR%\server\server.conf" (
    for /f "tokens=1,* delims==" %%a in ('findstr /b "PORT=" "%SCRIPT_DIR%\server\server.conf"') do set port=%%b
)
if not "%~1"=="" set port=%~1

REM Define paths - gamedata is relative to script location
if exist "%SCRIPT_DIR%\gamedata" (
    set GAMEDATA_DIR=%SCRIPT_DIR%\gamedata
) else (
    set GAMEDATA_DIR=%SCRIPT_DIR%
)

REM Remove shutdown file if it exists
if exist "%GAMEDATA_DIR%\area\shutdown.txt" (
    del /q "%GAMEDATA_DIR%\area\shutdown.txt"
)

:loop
    REM Run the MUD (executable is in gamedata/)
    REM Logs are written internally to gamedata/log/ with timestamped filenames
    echo Starting Dystopia MUD on port %port%...
    "%GAMEDATA_DIR%\dystopia.exe" %port%

    REM Exit code 99 = copyover (parent exits after spawning child process)
    if %ERRORLEVEL% == 99 (
        echo Copyover completed, new server process is running.
        goto :end
    )

    REM Check for shutdown file
    if exist "%GAMEDATA_DIR%\area\shutdown.txt" (
        del /q "%GAMEDATA_DIR%\area\shutdown.txt"
        echo MUD shutdown complete.
        goto :end
    )

    REM Restart after brief pause
    echo MUD crashed or rebooted. Restarting in 2 seconds...
    timeout /t 2 /nobreak > nul
    goto :loop

:end
endlocal
