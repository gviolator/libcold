set BuildType=%1
set Arch=x86_64
set LibName=gtest

set SourceDir=%ThirdPartyPath%/googletest
set BuildDir=%ThirdPartyPath%/_build/%LibName%-%Arch%-%BuildType%
set InstallDir=%ThirdPartyPath%/_dist/%Arch%-%BuildType%/%LibName%

echo Configure [%LibName%]:%BuildType%

cmake ^
	-G "Visual Studio 16 2019" ^
	-A x64 ^
	-DCMAKE_BUILD_TYPE=%BuildType% ^
	-DCMAKE_C_COMPILER=cl ^
	-DCMAKE_CXX_COMPILER=cl ^
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES ^
	-Dgtest_force_shared_crt=YES ^
	-DBUILD_GMOCK=YES ^
	-DCMAKE_INSTALL_PREFIX=%InstallDir% ^
	-B %BuildDir% ^
	-S %SourceDir%

cmake --build "%BuildDir%" --target install --config %BuildType%
