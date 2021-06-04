call %~dp0\..\..\buildscripts\windows-setupenv.cmd
call %~dp0\..\..\buildscripts\windows\env_msvc2019.cmd x64 x64

set BuildType=%1
set Arch=x86_64
set LibName=cryptopp

set SourceDir=%ThirdPartyPath%/%LibName%
set BuildDir=%ThirdPartyPath%/_build/%LibName%-%Arch%-%BuildType%
set InstallDir=%ThirdPartyPath%/_dist/%Arch%-%BuildType%/%LibName%

echo Configure [%LibName%]:%BuildType%

cmake ^
	-G "Ninja" ^
	-DCMAKE_C_COMPILER=cl.exe ^
	-DCMAKE_CXX_COMPILER=cl.exe ^
	-DCMAKE_BUILD_TYPE=%BuildType% ^
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES ^
	-DCMAKE_INSTALL_PREFIX=%InstallDir% ^
	-B %BuildDir% ^
	-S %SourceDir%

rem cmake --build "%BuildDir%" --target install --config %BuildType%
cmake --build "%BuildDir%" --config %BuildType%
