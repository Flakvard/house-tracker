@echo off

REM Set command
set CMAKE_CMD=cmake -G "Visual Studio 17 2022" -A x64 -B ./buildVS .

REM Run command
echo %CMAKE_CMD%
%CMAKE_CMD%

pause
