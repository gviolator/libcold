set BuildType=%1
set Arch=x86_64
set LibName=libuv

set SourceDir=%ThirdPartyPath%/%LibName%
set BuildDir=%ThirdPartyPath%/_build/%LibName%-%Arch%-%BuildType%
set InstallDir=%ThirdPartyPath%/_dist/%Arch%-%BuildType%/%LibName%

echo Configure [%LibName%]:%BuildType%

cmake ^
	-G "Visual Studio 16 2019" ^
	-A x64 ^
	-DCMAKE_BUILD_TYPE=%BuildType% ^
	-DCMAKE_C_COMPILER=cl ^
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES ^
	-DCMAKE_INSTALL_PREFIX=%InstallDir% ^
	-DLIBUV_BUILD_TESTS=NO ^
	-DLIBUV_BUILD_BENCH=NO ^
	-B %BuildDir% ^
	-S %SourceDir%

cmake --build "%BuildDir%" --target install --config %BuildType%
