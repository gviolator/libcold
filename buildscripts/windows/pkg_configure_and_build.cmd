@echo off
pushd .

call %~dp0setupenv.cmd

set PKG_EDITABLE_CONFIGURATION=1
set ProfileName=%1
set TargetDirName=%2
rem set PATH=%LLVM%;%PATH%

if "%TargetDirName%" == "" (
	@set TargetDirName=%ProfileName%
)

@set BuildDir=%ProjectRootPath%.target\%TargetDirName%\
@set ConanProfile=%BuildScriptsPath%conan-profiles\%ProfileName%\

if not exist "%BuildDir%" (
	echo "create '%BuildDir%' build directory"
	mkdir %BuildDir%
)

echo Initialize target (%BuildDir%) with profile:(%ConanProfile%)
conan install %ProjectRootPath% --profile %ConanProfile% --install-folder %BuildDir%

if "%CONFIGURE_ONLY%" == "" (
	echo Configure and BUILD target.
	conan build %PROJECTROOT_PATH% --build-folder %BuildDir% --source-folder %PROJECTROOT_PATH%
) else (
	echo Configure ONLY target.
	conan build %ProjectRootPath% --configure --build-folder %BuildDir% --source-folder %ProjectRootPath%
)

popd
