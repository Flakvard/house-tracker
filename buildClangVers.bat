@echo off

REM Set Clang compilers
set CC=clang
set CXX=clang++

REM Set command
set CMAKE_CMD=cmake -G "Ninja" ^
  -DCMAKE_C_COMPILER=clang ^
  -DCMAKE_CXX_COMPILER=clang++ ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/MarniFalkvardJoensen/scoop/apps/vcpkg/current/scripts/buildsystems/vcpkg.cmake" ^
  -B ./buildNinja .

echo %CMAKE_CMD%
%CMAKE_CMD%

pause
