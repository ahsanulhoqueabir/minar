@echo off
rem Builds the no-dependency Win32 version (OpenGL + GLU only).
rem For the GLUT version see README.md.
setlocal
set "CXX=g++"
if exist "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" (
    set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"
)
%CXX% -O2 -Wall -o shaheed_minar.exe src\main_win32.cpp src\app.cpp src\minar_model.cpp src\geometry.cpp src\pigeons.cpp -lopengl32 -lglu32 -lgdi32 -mwindows -static
if errorlevel 1 exit /b 1
echo Built shaheed_minar.exe
