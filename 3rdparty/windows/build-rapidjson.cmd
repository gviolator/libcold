set BuildType=%1
set Arch=x86_64
set LibName=RapidJSON

set SourceDir=%ThirdPartyPath%/%LibName%
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
	-DRAPIDJSON_BUILD_CXX17=ON ^
	-DRAPIDJSON_BUILD_CXX11=OFF ^
	-DRAPIDJSON_BUILD_TESTS=OFF ^
	-DRAPIDJSON_BUILD_EXAMPLES=OFF ^
	-DRAPIDJSON_BUILD_DOC=OFF ^
	-DCMAKE_INSTALL_PREFIX=%InstallDir% ^
	-B %BuildDir% ^
	-S %SourceDir%

cmake --build "%BuildDir%" --target install --config %BuildType%
