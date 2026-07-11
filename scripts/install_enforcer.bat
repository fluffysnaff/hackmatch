@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Direct launches get their own persistent console. Automated callers pass
rem --same-window so they keep the original process and receive the exit code.
if /I "%~1"=="--child" goto :main
if /I "%~1"=="--same-window" goto :main
start "Hackmatch Installer" "%ComSpec%" /d /k ""%~f0" --child %*"
exit /b 0

:main
for /f "delims=" %%E in ('echo prompt $E^| "%ComSpec%"') do set "ESC=%%E"
set "RESET=%ESC%[0m"
set "CYAN=%ESC%[96m"
set "BLUE=%ESC%[94m"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"

for %%I in ("%~dp0..") do set "SOURCE_ROOT=%%~fI"
set "DLL=%SOURCE_ROOT%\build-release\hackmatch.dll"
if exist "%SOURCE_ROOT%\hackmatch.dll" set "DLL=%SOURCE_ROOT%\hackmatch.dll"
set "FINDER=%~dp0find_redmatch.ps1"
set "GAME_ARGUMENT="
set "DRY_RUN=0"

:parse_arguments
if "%~1"=="" goto :arguments_parsed
if /I "%~1"=="--child" (
    shift
    goto :parse_arguments
)
if /I "%~1"=="--same-window" (
    shift
    goto :parse_arguments
)
if /I "%~1"=="--dry-run" (
    set "DRY_RUN=1"
    shift
    goto :parse_arguments
)
if not defined GAME_ARGUMENT (
    set "GAME_ARGUMENT=%~1"
    shift
    goto :parse_arguments
)
call :error "Unexpected argument: %~1"
call :usage
exit /b 2

:arguments_parsed
echo.
echo %BLUE%  Hackmatch Installer%RESET%
echo %BLUE%  -------------------%RESET%

if not exist "%DLL%" (
    call :error "Missing build artifact: %DLL%"
    echo Run scripts\build_release.bat first.
    exit /b 1
)
powershell.exe -NoLogo -NoProfile -Command "$stream = [IO.File]::OpenRead($env:DLL); try { $first = $stream.ReadByte(); $second = $stream.ReadByte(); if ($first -ne 0x4D -or $second -ne 0x5A) { exit 1 } } finally { $stream.Dispose() }" >nul 2>&1
if errorlevel 1 (
    call :error "Invalid build artifact: %DLL%"
    echo Rebuild Hackmatch and try again.
    exit /b 1
)
if not exist "%FINDER%" (
    call :error "Missing discovery helper: %FINDER%"
    exit /b 1
)

set "GAME_ROOT="
if defined GAME_ARGUMENT call :discover "%GAME_ARGUMENT%"
if not defined GAME_ROOT call :discover

:prompt_for_game
if defined GAME_ROOT goto :game_found
call :warning "Redmatch 2 was not found in your Steam libraries."
echo Example: C:\Program Files (x86)\Steam\steamapps\common\Redmatch 2
set "MANUAL_PATH="
set /p "MANUAL_PATH=Enter the Redmatch 2 folder, or press Enter to cancel: "
if not defined MANUAL_PATH (
    call :warning "Installation cancelled."
    exit /b 1
)
call :discover "%MANUAL_PATH%"
if not defined GAME_ROOT (
    call :error "That folder does not contain Redmatch 2.exe and Redmatch 2_Data."
    goto :prompt_for_game
)

:game_found
set "TARGET=%GAME_ROOT%\Redmatch 2_Data"
set "OUTPUT=%TARGET%\enforcer.dll"
set "TEMP_OUTPUT=%TARGET%\enforcer.dll.hackmatch.tmp"

call :info "Game: %GAME_ROOT%"

if "%DRY_RUN%"=="1" (
    call :info "Source: %DLL%"
    call :info "Destination: %OUTPUT%"
    call :success "Installation preview completed; no files were changed."
    exit /b 0
)

if exist "%TEMP_OUTPUT%" del /f /q "%TEMP_OUTPUT%" >nul 2>&1
copy /y "%DLL%" "%TEMP_OUTPUT%" >nul
if errorlevel 1 (
    call :error "Could not copy the DLL into the game folder."
    echo Check folder permissions and try again.
    exit /b 1
)

move /y "%TEMP_OUTPUT%" "%OUTPUT%" >nul
if errorlevel 1 (
    del /f /q "%TEMP_OUTPUT%" >nul 2>&1
    call :error "Could not replace enforcer.dll."
    echo Close Redmatch 2 or unload the existing DLL, then try again.
    exit /b 1
)

call :success "Hackmatch installed successfully."
echo %CYAN%Source:%RESET%      %DLL%
echo %CYAN%Destination:%RESET% %OUTPUT%
exit /b 0

:discover
set "FOUND_ROOT="
if "%~1"=="" (
    for /f "usebackq delims=" %%I in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%FINDER%" 2^>nul`) do set "FOUND_ROOT=%%I"
) else (
    for /f "usebackq delims=" %%I in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%FINDER%" -ExplicitPath "%~1" 2^>nul`) do set "FOUND_ROOT=%%I"
)
if defined FOUND_ROOT set "GAME_ROOT=%FOUND_ROOT%"
exit /b 0

:usage
echo Usage: install_enforcer.bat [game-root] [--same-window] [--dry-run]
exit /b 0

:info
echo %CYAN%[INFO]%RESET% %~1
exit /b 0

:success
echo %GREEN%[ OK ]%RESET% %~1
exit /b 0

:warning
echo %YELLOW%[WARN]%RESET% %~1
exit /b 0

:error
echo %RED%[FAIL]%RESET% %~1
exit /b 0
