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
    REM Find next available log file number
    set index=1000
    :findlog
        set logfile=%GAMEDATA_DIR%\log\!index!.log
        if not exist "!logfile!" goto :runmud
        set /a index+=1
        goto :findlog

    :runmud
    REM Run the MUD (executable is in gamedata/)
    echo Starting Dystopia MUD on port %port%...
    echo Log file: !logfile!
    "%GAMEDATA_DIR%\dystopia.exe" %port% > "!logfile!" 2>&1

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
