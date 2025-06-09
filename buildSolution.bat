@echo off
REM ——————————————————————————————
REM First install the packages in vcpkg.json
REM vcpkg install
REM 1) Where vcpkg.exe actually lives on Windows:
set VCPKG_ROOT=C:/Users/marni/scoop/apps/vcpkg/current

cmake ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_MANIFEST_MODE=ON ^
  -B .\buildVS ^
  .

pause
