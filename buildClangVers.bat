@echo off
REM ── Bring MSVC & Windows-SDK paths into %PATH%
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"

set CC=clang-cl
set CXX=clang-cl

cmake -G Ninja ^
  -DCMAKE_C_COMPILER=%CC% ^
  -DCMAKE_CXX_COMPILER=%CXX% ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/MarniFalkvardJoensen/scoop/apps/vcpkg/current/scripts/buildsystems/vcpkg.cmake" ^
  -B build -S .

ninja -C build         
REM or: cmake --build build
