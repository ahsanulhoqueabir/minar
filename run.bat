@echo off
rem Dev run: close old window, rebuild everything, launch. Used by Code Runner.
cd /d "%~dp0"
if exist "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"
taskkill /f /im shaheed_minar_dev.exe >nul 2>&1
g++ -O1 -Wall -o shaheed_minar_dev.exe src\main_win32.cpp src\app.cpp src\minar_model.cpp src\geometry.cpp src\pigeons.cpp -lopengl32 -lglu32 -lgdi32 -mwindows -static
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
echo Built OK, launching...
start "" shaheed_minar_dev.exe
