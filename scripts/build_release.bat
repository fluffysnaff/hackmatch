@echo off
setlocal

for %%I in ("%~dp0..") do set "SRC=%%~fI"
set "BUILD=%SRC%\build-release"
set "VSDEVCMD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

if exist "%VSDEVCMD%" call "%VSDEVCMD%" -arch=x64 >nul

cmake -S "%SRC%" -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DFETCHCONTENT_FULLY_DISCONNECTED=OFF
if errorlevel 1 exit /b 1

cmake --build "%BUILD%" --config Release
if errorlevel 1 exit /b 1

echo Built "%BUILD%\hackmatch.dll"
