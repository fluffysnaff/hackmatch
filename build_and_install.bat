@echo off
setlocal

call "%~dp0scripts\build_release.bat"
if errorlevel 1 exit /b 1

call "%~dp0scripts\install_enforcer.bat" --same-window %*
if errorlevel 1 exit /b 1
