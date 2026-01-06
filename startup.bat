@echo off
REM Written by Furey.
REM With additions from Tony.
REM Converted to batch by Claude.

setlocal enabledelayedexpansion

REM Set the port number (default 8888, or pass as argument)
set port=8888
if not "%~1"=="" set port=%~1

REM Get the directory where this script lives
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

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
